#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include "gtest/gtest.h"

#include "H5Cpp.h"

#define abstol 1e-6

using namespace H5;

int global_argc;
char** global_argv;

class TestHDF : public ::testing::Test
{
    protected:
    TestHDF()
    {
        if(global_argc<=2)
        {
        std::cout<<"Not enough arguments given!\n";
        std::abort();
        }
        else
        {
            std::string filename_1 = global_argv[1];
            std::string filename_2 = global_argv[2];
            if(filename_1.find("py")!=std::string::npos)
            {
                pyname = filename_1;
                cppname = filename_2;
            }
            else if(filename_1.find("cpp")!=std::string::npos)
            {
                cppname = filename_1;
                pyname = filename_2;
            }
            else
            {
                std::cout<<"Failed to open. In argv *cpp.hdf5 and *py.hdf5 file needed";
                std::abort();
            }
            cppfile=H5File(cppname,H5F_ACC_RDONLY);
            pyfile=H5File(pyname,H5F_ACC_RDONLY);
        }
        mytype=CompType(sizeof(instanceof));
        mytype.insertMember( "r", HOFFSET(ctype, real), PredType::NATIVE_DOUBLE);
        mytype.insertMember( "i", HOFFSET(ctype, imag), PredType::NATIVE_DOUBLE);
    }
    virtual ~TestHDF()
    {
        cppfile.close();
        pyfile.close();
        mytype.close();
    }
    void SetUp()
    {
        Group block_0 = cppfile.openGroup("/datablock_0");
        Attribute apacket = block_0.openAttribute("packet");
        Attribute aenergy = block_0.openAttribute("energy");
        Attribute anorm = block_0.openAttribute("norm");
        Attribute adt_cpp = block_0.openAttribute("dt");
        apacket.read(PredType::NATIVE_INT,&bool_packet);
        aenergy.read(PredType::NATIVE_INT,&bool_energy);
        anorm.read(PredType::NATIVE_INT,&bool_norm);
        adt_cpp.read(PredType::NATIVE_DOUBLE,&dt_cpp);
        apacket.close();
        aenergy.close();
        anorm.close();
        adt_cpp.close();
        block_0.close();

        block_0 = pyfile.openGroup("/datablock_0");
        Attribute adt_py = block_0.openAttribute("ext:dt");
        StrType mystrtype=adt_py.getStrType();
        H5std_string buff_dt_py="";
        adt_py.read(mystrtype,buff_dt_py);
        dt_py=std::strtod(buff_dt_py.c_str(),NULL);
        mystrtype.close();
        adt_py.close();
        block_0.close();

    }
    void TearDown()
    {

    }
    void time_matching(std::vector<int*>& match,H5std_string timepath_cpp,H5std_string timepath_py)
    {
        DataSet its_cpp;
        DataSet its_py;
        DataSpace itsp_cpp;
        DataSpace itsp_py;

        its_cpp=cppfile.openDataSet(timepath_cpp);
        its_py=pyfile.openDataSet(timepath_py);
        itsp_cpp=its_cpp.getSpace();
        itsp_py=its_py.getSpace();

        int itrank_cpp=itsp_cpp.getSimpleExtentNdims();
        int itrank_py=itsp_py.getSimpleExtentNdims();

        hsize_t* itdim_cpp=new hsize_t[itrank_cpp];
        hsize_t* itdim_py=new hsize_t[itrank_py];

        IntType it_cpp_tp=its_cpp.getIntType();
        IntType it_py_tp=its_py.getIntType();

        itsp_cpp.getSimpleExtentDims(itdim_cpp);
        itsp_py.getSimpleExtentDims(itdim_py);

        hsize_t* elemt=new hsize_t[itrank_cpp];
        elemt[0]=0;
        hsize_t* elemnt=new hsize_t[itrank_cpp];
        elemnt[0]=1;

        DataSpace itelem(itrank_cpp,elemnt);
        itelem.selectHyperslab(H5S_SELECT_SET,elemnt,elemt);

        hsize_t* dyn_start=new hsize_t[itrank_cpp];

        int* read_data_cpp=new int[itdim_cpp[0]];
        int* read_data_py=new int[itdim_py[0]];

        for(unsigned int k=0;k<itdim_cpp[0];++k)
        {
            dyn_start[0]=k;
            itsp_cpp.selectHyperslab(H5S_SELECT_SET,elemnt,dyn_start);
            its_cpp.read(&read_data_cpp[k],it_cpp_tp,itelem,itsp_cpp);
        }
        for(unsigned int j=0;j<itdim_py[0];++j)
        {
            dyn_start[0]=j;
            itsp_py.selectHyperslab(H5S_SELECT_SET,elemnt,dyn_start);
            its_py.read(&read_data_py[j],it_py_tp,itelem,itsp_py);
        }

        itelem.close();
        it_cpp_tp.close();
        it_py_tp.close();
        itsp_cpp.close();
        itsp_py.close();
        its_cpp.close();
        its_py.close();


        int acc[2]={-1,-1};
        for(unsigned int k=0;k<itdim_cpp[0];++k)
        {
            int index_i_cpp=read_data_cpp[k];
            for(unsigned int p=0;p<itdim_py[0];++p)
            {
                int index_j_py=read_data_py[p];
                int temp = index_j_py*(dt_py/dt_cpp);
                if(index_i_cpp==temp)
                {
                    int *elem =new int[2];
                    elem[0]=k;
                    elem[1]=p;
                    match.push_back(elem);
                    break;
                }
            }
        }
        if(match.empty())
        {
            match.push_back(acc);
        }
        delete [] read_data_cpp;
        delete [] read_data_py;

        itsp_cpp.close();
        itsp_py.close();
        its_cpp.close();
        its_py.close();
    }
    struct ctype{
        double real=0.;
        double imag=0.;
    } instanceof;
    H5File cppfile;
    H5File pyfile;
    H5std_string cppname;
    H5std_string pyname;
    int bool_packet;
    int bool_energy;
    int bool_norm;
    CompType mytype;
    double dt_cpp;
    double dt_py;
};

TEST_F(TestHDF,Testpacket)
{
    if(bool_packet)
    {
        H5std_string timepath_cpp="/datablock_0/wavepacket/timegrid";
        H5std_string timepath_py="/datablock_0/wavepacket/timegrid";
        std::vector<int*> time_matching_packet;
        time_matching(time_matching_packet,timepath_cpp,timepath_py);

        H5std_string path_Q="/datablock_0/wavepacket/Pi/Q";
        ctype* Q_cpp;
        ctype* Q_py;

        DataSet ds_cpp;
        DataSet ds_py;
        DataSpace sp_cpp;
        DataSpace sp_py;

        ds_cpp=cppfile.openDataSet(path_Q);
        ds_py=pyfile.openDataSet(path_Q);

        sp_cpp=ds_cpp.getSpace();
        sp_py=ds_py.getSpace();

        int Q_rank_cpp=sp_cpp.getSimpleExtentNdims();
        int Q_rank_py=sp_py.getSimpleExtentNdims();

        hsize_t* Q_dim_cpp=new hsize_t[Q_rank_cpp];
        hsize_t* Q_dim_py=new hsize_t[Q_rank_py];

        sp_cpp.getSimpleExtentDims(Q_dim_cpp);
        sp_py.getSimpleExtentDims(Q_dim_py);

        CompType Q_type_cpp = ds_cpp.getCompType();
        CompType Q_type_py = ds_py.getCompType();

        int index_Q_cpp=Q_dim_cpp[1]*Q_dim_cpp[2];
        int index_Q_py=Q_dim_py[1]*Q_dim_py[2];
        ctype* tmp1 = new ctype[Q_dim_cpp[0]*index_Q_cpp];
        ctype* tmp2 = new ctype[Q_dim_py[0]*index_Q_py];
        Q_cpp=tmp1;
        Q_py=tmp2;

        ASSERT_EQ(Q_dim_cpp[1],Q_dim_py[1])<<"Dimension mismatch. Abort!";
        ASSERT_EQ(Q_dim_cpp[2],Q_dim_py[2])<<"Dimension mismatch. Abort!";

        hsize_t* Q_elem=new hsize_t[Q_rank_cpp];
        Q_elem[0]=1;
        Q_elem[1]=Q_dim_cpp[1];
        Q_elem[2]=Q_dim_cpp[2];
        hsize_t* Q_elem_start=new hsize_t[Q_rank_cpp];
        Q_elem_start[0]=0;Q_elem_start[1]=0;Q_elem_start[2]=0;
        hsize_t* Q_elem_count=new hsize_t[Q_rank_cpp];
        Q_elem_count[0]=1;Q_elem_count[1]=Q_dim_cpp[1];Q_elem_count[2]=Q_dim_cpp[2];

        DataSpace Q_elem_sp(Q_rank_cpp,Q_elem);
        Q_elem_sp.selectHyperslab(H5S_SELECT_SET,Q_elem_count,Q_elem_start);

        hsize_t* Q_dyn_start=new hsize_t[Q_rank_cpp];
        for(unsigned int k=0;k<Q_dim_cpp[0];++k)
        {
            Q_dyn_start[0]=k;
            Q_dyn_start[1]=0;
            Q_dyn_start[2]=0;
            sp_cpp.selectHyperslab(H5S_SELECT_SET,Q_elem_count,Q_dyn_start);
            ds_cpp.read(&(Q_cpp[index_Q_cpp*k]),Q_type_cpp,Q_elem_sp,sp_cpp);
        }
        for(unsigned int j=0;j<Q_dim_py[0];++j)
        {
            Q_dyn_start[0]=j;
            Q_dyn_start[1]=0;
            Q_dyn_start[2]=0;
            sp_py.selectHyperslab(H5S_SELECT_SET,Q_elem_count,Q_dyn_start);
            ds_py.read(&(Q_py[index_Q_py*j]),Q_type_py,Q_elem_sp,sp_py);
        }
        Q_type_cpp.close();
        Q_type_py.close();
        sp_cpp.close();
        sp_py.close();
        ds_cpp.close();
        ds_py.close();

        if(time_matching_packet.empty())
        {
            ADD_FAILURE() << "No matching timepoints for this packet. Abort!";
        }
        else if(time_matching_packet[0][0]==-1)
        {
            ADD_FAILURE() << "No matching timepoints for this packet. Abort!";
        }
        else
        {
            std::cout<<time_matching_packet.size()<<" matching datapoints for packet.\n";
            for(auto index_elem:time_matching_packet)
            {
                for(int p=0;p<index_Q_cpp;++p)
                {
                    EXPECT_NEAR(Q_cpp[index_Q_cpp*index_elem[0]+p].real,Q_py[index_Q_py*index_elem[1]+p].real,abstol);
                    EXPECT_NEAR(Q_cpp[index_Q_cpp*index_elem[0]+p].imag,Q_py[index_Q_py*index_elem[1]+p].imag,abstol);
                }

            }
        }

        H5std_string path_P="/datablock_0/wavepacket/Pi/P";
        DataSet ds2_cpp;
        DataSet ds2_py;
        DataSpace sp2_cpp;
        DataSpace sp2_py;

        ds2_cpp=cppfile.openDataSet(path_P);
        ds2_py=pyfile.openDataSet(path_P);

        sp2_cpp=ds2_cpp.getSpace();
        sp2_py=ds2_py.getSpace();

        int P_rank_cpp=sp2_cpp.getSimpleExtentNdims();
        int P_rank_py=sp2_py.getSimpleExtentNdims();

        hsize_t* P_dim_cpp=new hsize_t[P_rank_cpp];
        hsize_t* P_dim_py=new hsize_t[P_rank_py];

        sp2_cpp.getSimpleExtentDims(P_dim_cpp);
        sp2_py.getSimpleExtentDims(P_dim_py);

        CompType P_type_cpp = ds2_cpp.getCompType();
        CompType P_type_py = ds2_py.getCompType();

        int index_P_cpp=P_dim_cpp[1]*P_dim_cpp[2];
        int index_P_py=P_dim_py[1]*P_dim_py[2];
        ctype* P_cpp = new ctype[P_dim_cpp[0]*index_P_cpp];
        ctype* P_py = new ctype[P_dim_py[0]*index_P_py];

        ASSERT_EQ(P_dim_cpp[1],P_dim_py[1])<<"Dimension mismatch. Abort!";
        ASSERT_EQ(P_dim_cpp[2],P_dim_py[2])<<"Dimension mismatch. Abort!";

        hsize_t* P_elem=new hsize_t[P_rank_cpp];
        P_elem[0]=1;
        P_elem[1]=P_dim_cpp[1];
        P_elem[2]=P_dim_cpp[2];
        hsize_t* P_elem_start=new hsize_t[P_rank_cpp];
        P_elem_start[0]=0;P_elem_start[1]=0;P_elem_start[2]=0;
        hsize_t* P_elem_count=new hsize_t[P_rank_cpp];
        P_elem_count[0]=1;P_elem_count[1]=P_dim_cpp[1];P_elem_count[2]=P_dim_cpp[2];

        DataSpace P_elem_sp(P_rank_cpp,P_elem);
        P_elem_sp.selectHyperslab(H5S_SELECT_SET,P_elem_count,P_elem_start);

        hsize_t* P_dyn_start=new hsize_t[P_rank_cpp];
        for(unsigned int k=0;k<P_dim_cpp[0];++k)
        {
            P_dyn_start[0]=k;
            P_dyn_start[1]=0;
            P_dyn_start[2]=0;
            sp2_cpp.selectHyperslab(H5S_SELECT_SET,P_elem_count,P_dyn_start);
            ds2_cpp.read(&(P_cpp[index_P_cpp*k]),P_type_cpp,P_elem_sp,sp2_cpp);
        }
        for(unsigned int j=0;j<P_dim_py[0];++j)
        {
            P_dyn_start[0]=j;
            P_dyn_start[1]=0;
            P_dyn_start[2]=0;
            sp2_py.selectHyperslab(H5S_SELECT_SET,P_elem_count,P_dyn_start);
            ds2_py.read(&(P_py[index_P_py*j]),P_type_py,P_elem_sp,sp2_py);
        }

        P_type_cpp.close();
        P_type_py.close();
        sp2_cpp.close();
        sp2_py.close();
        ds2_cpp.close();
        ds2_py.close();

        for(auto index_elem:time_matching_packet)
        {
            for(int p=0;p<index_P_cpp;++p)
            {
                EXPECT_NEAR(P_cpp[index_P_cpp*index_elem[0]+p].real,P_py[index_P_py*index_elem[1]+p].real,abstol);
                EXPECT_NEAR(P_cpp[index_P_cpp*index_elem[0]+p].imag,P_py[index_P_py*index_elem[1]+p].imag,abstol);
            }
        }


        H5std_string path_q="/datablock_0/wavepacket/Pi/q";
        DataSet ds3_cpp;
        DataSet ds3_py;
        DataSpace sp3_cpp;
        DataSpace sp3_py;

        ds3_cpp=cppfile.openDataSet(path_q);
        ds3_py=pyfile.openDataSet(path_q);

        sp3_cpp=ds3_cpp.getSpace();
        sp3_py=ds3_py.getSpace();

        int q_rank_cpp=sp3_cpp.getSimpleExtentNdims();
        int q_rank_py=sp3_py.getSimpleExtentNdims();

        hsize_t* q_dim_cpp=new hsize_t[q_rank_cpp];
        hsize_t* q_dim_py=new hsize_t[q_rank_py];

        sp3_cpp.getSimpleExtentDims(q_dim_cpp);
        sp3_py.getSimpleExtentDims(q_dim_py);

        CompType q_type_cpp = ds3_cpp.getCompType();
        CompType q_type_py = ds3_py.getCompType();

        int index_q_cpp=q_dim_cpp[1]*q_dim_cpp[2];
        int index_q_py=q_dim_py[1]*q_dim_py[2];
        ctype* q_cpp = new ctype[q_dim_cpp[0]*index_q_cpp];
        ctype* q_py = new ctype[q_dim_py[0]*index_q_py];

        ASSERT_EQ(q_dim_cpp[1],q_dim_py[1])<<"Dimension mismatch. Abort!";
        ASSERT_EQ(q_dim_cpp[2],q_dim_py[2])<<"Dimension mismatch. Abort!";

        hsize_t* q_elem=new hsize_t[q_rank_cpp];
        q_elem[0]=1;
        q_elem[1]=q_dim_cpp[1];
        q_elem[2]=q_dim_cpp[2];
        hsize_t* q_elem_start=new hsize_t[q_rank_cpp];
        q_elem_start[0]=0;q_elem_start[1]=0;q_elem_start[2]=0;
        hsize_t* q_elem_count=new hsize_t[q_rank_cpp];
        q_elem_count[0]=1;q_elem_count[1]=q_dim_cpp[1];q_elem_count[2]=q_dim_cpp[2];

        DataSpace q_elem_sp(q_rank_cpp,q_elem);
        q_elem_sp.selectHyperslab(H5S_SELECT_SET,q_elem_count,q_elem_start);

        hsize_t* q_dyn_start=new hsize_t[q_rank_cpp];

        for(unsigned int k=0;k<q_dim_cpp[0];++k)
        {
            q_dyn_start[0]=k;
            q_dyn_start[1]=0;
            q_dyn_start[2]=0;
            sp3_cpp.selectHyperslab(H5S_SELECT_SET,q_elem_count,q_dyn_start);
            ds3_cpp.read(&(q_cpp[index_q_cpp*k]),q_type_cpp,q_elem_sp,sp3_cpp);
        }
        for(unsigned int j=0;j<q_dim_py[0];++j)
        {
            q_dyn_start[0]=j;
            q_dyn_start[1]=0;
            q_dyn_start[2]=0;
            sp3_py.selectHyperslab(H5S_SELECT_SET,q_elem_count,q_dyn_start);
            ds3_py.read(&(q_py[index_q_py*j]),q_type_py,q_elem_sp,sp3_py);
        }

        q_type_cpp.close();
        q_type_py.close();
        sp3_cpp.close();
        sp3_py.close();
        ds3_cpp.close();
        ds3_py.close();

        for(auto index_elem:time_matching_packet)
        {
            for(int p=0;p<index_q_cpp;++p)
            {
                EXPECT_NEAR(q_cpp[index_q_cpp*index_elem[0]+p].real,q_py[index_q_py*index_elem[1]+p].real,abstol);
                EXPECT_NEAR(q_cpp[index_q_cpp*index_elem[0]+p].imag,q_py[index_q_py*index_elem[1]+p].imag,abstol);
            }
        }

        H5std_string path_p="/datablock_0/wavepacket/Pi/p";
        DataSet ds4_cpp;
        DataSet ds4_py;
        DataSpace sp4_cpp;
        DataSpace sp4_py;

        ds4_cpp=cppfile.openDataSet(path_p);
        ds4_py=pyfile.openDataSet(path_p);

        sp4_cpp=ds4_cpp.getSpace();
        sp4_py=ds4_py.getSpace();

        int p_rank_cpp=sp4_cpp.getSimpleExtentNdims();
        int p_rank_py=sp4_py.getSimpleExtentNdims();

        hsize_t* p_dim_cpp=new hsize_t[p_rank_cpp];
        hsize_t* p_dim_py=new hsize_t[p_rank_py];

        sp4_cpp.getSimpleExtentDims(p_dim_cpp);
        sp4_py.getSimpleExtentDims(p_dim_py);

        CompType p_type_cpp = ds4_cpp.getCompType();
        CompType p_type_py = ds4_py.getCompType();

        int index_p_cpp=p_dim_cpp[1]*p_dim_cpp[2];
        int index_p_py=p_dim_py[1]*p_dim_py[2];
        ctype* p_cpp = new ctype[p_dim_cpp[0]*index_p_cpp];
        ctype* p_py = new ctype[p_dim_py[0]*index_p_py];

        ASSERT_EQ(p_dim_cpp[1],p_dim_py[1])<<"Dimension mismatch. Abort!";
        ASSERT_EQ(p_dim_cpp[2],p_dim_py[2])<<"Dimension mismatch. Abort!";

        hsize_t* p_elem=new hsize_t[p_rank_cpp];
        p_elem[0]=1;
        p_elem[1]=p_dim_cpp[1];
        p_elem[2]=p_dim_cpp[2];
        hsize_t* p_elem_start=new hsize_t[p_rank_cpp];
        p_elem_start[0]=0;p_elem_start[1]=0;p_elem_start[2]=0;
        hsize_t* p_elem_count=new hsize_t[p_rank_cpp];
        p_elem_count[0]=1;p_elem_count[1]=p_dim_cpp[1];p_elem_count[2]=p_dim_cpp[2];

        DataSpace p_elem_sp(p_rank_cpp,p_elem);
        p_elem_sp.selectHyperslab(H5S_SELECT_SET,p_elem_count,p_elem_start);

        hsize_t* p_dyn_start=new hsize_t[p_rank_cpp];

        for(unsigned int k=0;k<p_dim_cpp[0];++k)
        {
            p_dyn_start[0]=k;
            p_dyn_start[1]=0;
            p_dyn_start[2]=0;
            sp4_cpp.selectHyperslab(H5S_SELECT_SET,p_elem_count,p_dyn_start);
            ds4_cpp.read(&(p_cpp[index_p_cpp*k]),p_type_cpp,p_elem_sp,sp4_cpp);
        }
        for(unsigned int j=0;j<p_dim_py[0];++j)
        {
            p_dyn_start[0]=j;
            p_dyn_start[1]=0;
            p_dyn_start[2]=0;
            sp4_py.selectHyperslab(H5S_SELECT_SET,p_elem_count,p_dyn_start);
            ds4_py.read(&(p_py[index_p_py*j]),p_type_py,p_elem_sp,sp4_py);
        }

        p_type_cpp.close();
        p_type_py.close();
        sp4_cpp.close();
        sp4_py.close();
        ds4_cpp.close();
        ds4_py.close();

        for(auto index_elem:time_matching_packet)
        {
            for(int p=0;p<index_p_cpp;++p)
            {
                EXPECT_NEAR(p_cpp[index_p_cpp*index_elem[0]+p].real,p_py[index_p_py*index_elem[1]+p].real,abstol);
                EXPECT_NEAR(p_cpp[index_p_cpp*index_elem[0]+p].imag,p_py[index_p_py*index_elem[1]+p].imag,abstol);
            }
        }

        H5std_string path_coeffs="/datablock_0/wavepacket/coefficients/c_0";
        DataSet ds5_cpp;
        DataSet ds5_py;
        DataSpace sp5_cpp;
        DataSpace sp5_py;

        ds5_cpp=cppfile.openDataSet(path_coeffs);
        ds5_py=pyfile.openDataSet(path_coeffs);

        sp5_cpp=ds5_cpp.getSpace();
        sp5_py=ds5_py.getSpace();

        int c_rank_cpp=sp5_cpp.getSimpleExtentNdims();
        int c_rank_py=sp5_py.getSimpleExtentNdims();

        hsize_t* c_dim_cpp=new hsize_t[c_rank_cpp];
        hsize_t* c_dim_py=new hsize_t[c_rank_py];

        sp5_cpp.getSimpleExtentDims(c_dim_cpp);
        sp5_py.getSimpleExtentDims(c_dim_py);

        CompType c_type_cpp = ds5_cpp.getCompType();
        CompType c_type_py = ds5_py.getCompType();

        int index_c_cpp=c_dim_cpp[1];
        int index_c_py=c_dim_py[1];
        ctype* c_cpp = new ctype[c_dim_cpp[0]*index_c_cpp];
        ctype* c_py = new ctype[c_dim_py[0]*index_c_py];

        ASSERT_EQ(c_dim_cpp[1],c_dim_py[1])<<"Dimension mismatch. Abort!";

        hsize_t* c_elem=new hsize_t[c_rank_cpp];
        c_elem[0]=1;
        c_elem[1]=c_dim_cpp[1];
        hsize_t* c_elem_start=new hsize_t[c_rank_cpp];
        c_elem_start[0]=0;c_elem_start[1]=0;
        hsize_t* c_elem_count=new hsize_t[c_rank_cpp];
        c_elem_count[0]=1;c_elem_count[1]=c_dim_cpp[1];

        DataSpace c_elem_sp(c_rank_cpp,c_elem);
        c_elem_sp.selectHyperslab(H5S_SELECT_SET,c_elem_count,c_elem_start);

        hsize_t* c_dyn_start=new hsize_t[c_rank_cpp];

        for(unsigned int k=0;k<c_dim_cpp[0];++k)
        {
            c_dyn_start[0]=k;
            c_dyn_start[1]=0;
            sp5_cpp.selectHyperslab(H5S_SELECT_SET,c_elem_count,c_dyn_start);
            ds5_cpp.read(&(c_cpp[index_c_cpp*k]),c_type_cpp,c_elem_sp,sp5_cpp);
        }
        for(unsigned int j=0;j<c_dim_py[0];++j)
        {
            c_dyn_start[0]=j;
            c_dyn_start[1]=0;
            sp5_py.selectHyperslab(H5S_SELECT_SET,c_elem_count,c_dyn_start);
            ds5_py.read(&(c_py[index_c_py*j]),c_type_py,c_elem_sp,sp5_py);
        }

        c_type_cpp.close();
        c_type_py.close();
        sp5_cpp.close();
        sp5_py.close();
        ds5_cpp.close();
        ds5_py.close();

        for(auto index_elem:time_matching_packet)
        {
            for(int p=0;p<index_c_cpp;++p)
            {
                EXPECT_NEAR(c_cpp[index_c_cpp*index_elem[0]+p].real,c_py[index_c_py*index_elem[1]+p].real,abstol);
                EXPECT_NEAR(c_cpp[index_c_cpp*index_elem[0]+p].imag,c_py[index_c_py*index_elem[1]+p].imag,abstol);
            }
        }

    }
    else
    {
        ADD_FAILURE() << "Packet data missing cannot compare. Abort!";
    }
}
TEST_F(TestHDF,DISABLED_Testenergies)
{
    if(bool_energy)
    {
        //ekin
        H5std_string timepath_cpp="/datablock_0/energies/timegrid_ekin";
        H5std_string timepath_py="/datablock_0/energies/timegrid_ekin";
        std::vector<int*> time_matching_ekin;
        time_matching(time_matching_ekin,timepath_cpp,timepath_py);

        H5std_string path_ekin="/datablock_0/energies/ekin";
        DataSet ds1_cpp;
        DataSet ds1_py;
        DataSpace sp1_cpp;
        DataSpace sp1_py;

        ds1_cpp=cppfile.openDataSet(path_ekin);
        ds1_py=pyfile.openDataSet(path_ekin);

        sp1_cpp=ds1_cpp.getSpace();
        sp1_py=ds1_py.getSpace();

        int ekin_rank_cpp=sp1_cpp.getSimpleExtentNdims();
        int ekin_rank_py=sp1_py.getSimpleExtentNdims();

        hsize_t* ekin_dim_cpp=new hsize_t[ekin_rank_cpp];
        hsize_t* ekin_dim_py=new hsize_t[ekin_rank_py];

        sp1_cpp.getSimpleExtentDims(ekin_dim_cpp);
        sp1_py.getSimpleExtentDims(ekin_dim_py);

         DataType ekin_type_cpp = ds1_cpp.getDataType();
         DataType ekin_type_py = ds1_py.getDataType();

         double* ekin_cpp=new double[ekin_dim_cpp[0]];
         double* ekin_py=new double[ekin_dim_py[0]];

         hsize_t* ekin_elem=new hsize_t[ekin_rank_cpp];
         ekin_elem[0]=1;
         ekin_elem[1]=1;
         hsize_t* ekin_elem_start=new hsize_t[ekin_rank_cpp];
         ekin_elem_start[0]=0;ekin_elem_start[1]=0;
         hsize_t* ekin_elem_count=new hsize_t[ekin_rank_cpp];
         ekin_elem_count[0]=1;ekin_elem_count[1]=1;

         DataSpace ekin_elem_sp(ekin_rank_cpp,ekin_elem);
         ekin_elem_sp.selectHyperslab(H5S_SELECT_SET,ekin_elem_count,ekin_elem_start);

         hsize_t* ekin_dyn_start=new hsize_t[ekin_rank_cpp];

         for(unsigned int k=0;k<ekin_dim_cpp[0];++k)
         {
             ekin_dyn_start[0]=k;
             ekin_dyn_start[1]=0;
             sp1_cpp.selectHyperslab(H5S_SELECT_SET,ekin_elem_count,ekin_dyn_start);
             ds1_cpp.read(&(ekin_cpp[k]),ekin_type_cpp,ekin_elem_sp,sp1_cpp);
         }
         for(unsigned int j=0;j<ekin_dim_py[0];++j)
         {
             ekin_dyn_start[0]=j;
             ekin_dyn_start[1]=0;
             sp1_py.selectHyperslab(H5S_SELECT_SET,ekin_elem_count,ekin_dyn_start);
             ds1_py.read(&(ekin_py[j]),ekin_type_py,ekin_elem_sp,sp1_py);
         }

         ekin_type_cpp.close();
         ekin_type_py.close();
         sp1_cpp.close();
         sp1_py.close();
         ds1_cpp.close();
         ds1_py.close();

         if(time_matching_ekin.empty())
         {
             ADD_FAILURE() << "No matching timepoints for ekin. Abort!";
         }
         else if(time_matching_ekin[0][0]==-1)
         {
             ADD_FAILURE() << "No matching timepoints for ekin. Abort!";
         }
         else
         {
             for(auto index_elem:time_matching_ekin)
             {
                  EXPECT_NEAR(ekin_cpp[index_elem[0]],ekin_py[index_elem[1]],abstol);
             }
         }

         //epot
         H5std_string timepath_epot_cpp="/datablock_0/energies/timegrid_epot";
         H5std_string timepath_epot_py="/datablock_0/energies/timegrid_epot";
         std::vector<int*> time_matching_epot;
         time_matching(time_matching_epot,timepath_epot_cpp,timepath_epot_py);

         H5std_string path_epot="/datablock_0/energies/epot";
         DataSet ds2_cpp;
         DataSet ds2_py;
         DataSpace sp2_cpp;
         DataSpace sp2_py;

         ds2_cpp=cppfile.openDataSet(path_epot);
         ds2_py=pyfile.openDataSet(path_epot);

         sp2_cpp=ds2_cpp.getSpace();
         sp2_py=ds2_py.getSpace();

         int epot_rank_cpp=sp2_cpp.getSimpleExtentNdims();
         int epot_rank_py=sp2_py.getSimpleExtentNdims();

         hsize_t* epot_dim_cpp=new hsize_t[epot_rank_cpp];
         hsize_t* epot_dim_py=new hsize_t[epot_rank_py];

         sp2_cpp.getSimpleExtentDims(epot_dim_cpp);
         sp2_py.getSimpleExtentDims(epot_dim_py);

          DataType epot_type_cpp = ds2_cpp.getDataType();
          DataType epot_type_py = ds2_py.getDataType();

          double* epot_cpp=new double[epot_dim_cpp[0]];
          double* epot_py=new double[epot_dim_py[0]];

          hsize_t* epot_elem=new hsize_t[epot_rank_cpp];
          epot_elem[0]=1;
          epot_elem[1]=1;
          hsize_t* epot_elem_start=new hsize_t[epot_rank_cpp];
          epot_elem_start[0]=0;epot_elem_start[1]=0;
          hsize_t* epot_elem_count=new hsize_t[epot_rank_cpp];
          epot_elem_count[0]=1;epot_elem_count[1]=1;

          DataSpace epot_elem_sp(epot_rank_cpp,epot_elem);
          epot_elem_sp.selectHyperslab(H5S_SELECT_SET,epot_elem_count,epot_elem_start);

          hsize_t* epot_dyn_start=new hsize_t[epot_rank_cpp];

          for(unsigned int k=0;k<epot_dim_cpp[0];++k)
          {
              epot_dyn_start[0]=k;
              epot_dyn_start[1]=0;
              sp2_cpp.selectHyperslab(H5S_SELECT_SET,epot_elem_count,epot_dyn_start);
              ds2_cpp.read(&(epot_cpp[k]),epot_type_cpp,epot_elem_sp,sp2_cpp);
          }
          for(unsigned int j=0;j<epot_dim_py[0];++j)
          {
              epot_dyn_start[0]=j;
              epot_dyn_start[1]=0;
              sp2_py.selectHyperslab(H5S_SELECT_SET,epot_elem_count,epot_dyn_start);
              ds2_py.read(&(epot_py[j]),epot_type_py,epot_elem_sp,sp2_py);
          }

          epot_type_cpp.close();
          epot_type_py.close();
          sp2_cpp.close();
          sp2_py.close();
          ds2_cpp.close();
          ds2_py.close();

          if(time_matching_epot.empty())
          {
              ADD_FAILURE() << "No matching timepoints for epot. Abort!";
          }
          else if(time_matching_epot[0][0]==-1)
          {
              ADD_FAILURE() << "No matching timepoints for epot. Abort!";
          }
          else
          {
              for(auto index_elem:time_matching_epot)
              {
                   EXPECT_NEAR(epot_cpp[index_elem[0]],epot_py[index_elem[1]],abstol);
              }
          }

    }
    else
    {
        ADD_FAILURE() << "Energy data missing cannot compare. Abort!";
    }
}

TEST_F(TestHDF,DISABLED_Testnorm)
{
    if(bool_norm)
    {
        H5std_string timepath_norm_cpp="/datablock_0/norm/timegrid";
        H5std_string timepath_norm_py="/datablock_0/norm/timegrid";
        std::vector<int*> time_matching_norm;
        time_matching(time_matching_norm,timepath_norm_cpp,timepath_norm_py);

        H5std_string path_norm="/datablock_0/norm/norm";
        DataSet ds3_cpp;
        DataSet ds3_py;
        DataSpace sp3_cpp;
        DataSpace sp3_py;

        ds3_cpp=cppfile.openDataSet(path_norm);
        ds3_py=pyfile.openDataSet(path_norm);

        sp3_cpp=ds3_cpp.getSpace();
        sp3_py=ds3_py.getSpace();

        int norm_rank_cpp=sp3_cpp.getSimpleExtentNdims();
        int norm_rank_py=sp3_py.getSimpleExtentNdims();

        hsize_t* norm_dim_cpp=new hsize_t[norm_rank_cpp];
        hsize_t* norm_dim_py=new hsize_t[norm_rank_py];

        sp3_cpp.getSimpleExtentDims(norm_dim_cpp);
        sp3_py.getSimpleExtentDims(norm_dim_py);

         DataType norm_type_cpp = ds3_cpp.getDataType();
         DataType norm_type_py = ds3_py.getDataType();

         double* norm_cpp=new double[norm_dim_cpp[0]];
         double* norm_py=new double[norm_dim_py[0]];

         hsize_t* norm_elem=new hsize_t[norm_rank_cpp];
         norm_elem[0]=1;
         norm_elem[1]=1;
         hsize_t* norm_elem_start=new hsize_t[norm_rank_cpp];
         norm_elem_start[0]=0;norm_elem_start[1]=0;
         hsize_t* norm_elem_count=new hsize_t[norm_rank_cpp];
         norm_elem_count[0]=1;norm_elem_count[1]=1;

         DataSpace norm_elem_sp(norm_rank_cpp,norm_elem);
         norm_elem_sp.selectHyperslab(H5S_SELECT_SET,norm_elem_count,norm_elem_start);

         hsize_t* norm_dyn_start=new hsize_t[norm_rank_cpp];

         for(unsigned int k=0;k<norm_dim_cpp[0];++k)
         {
             norm_dyn_start[0]=k;
             norm_dyn_start[1]=0;
             sp3_cpp.selectHyperslab(H5S_SELECT_SET,norm_elem_count,norm_dyn_start);
             ds3_cpp.read(&(norm_cpp[k]),norm_type_cpp,norm_elem_sp,sp3_cpp);
         }
         for(unsigned int j=0;j<norm_dim_py[0];++j)
         {
             norm_dyn_start[0]=j;
             norm_dyn_start[1]=0;
             sp3_py.selectHyperslab(H5S_SELECT_SET,norm_elem_count,norm_dyn_start);
             ds3_py.read(&(norm_py[j]),norm_type_py,norm_elem_sp,sp3_py);
         }

         norm_type_cpp.close();
         norm_type_py.close();
         sp3_cpp.close();
         sp3_py.close();
         ds3_cpp.close();
         ds3_py.close();

         if(time_matching_norm.empty())
         {
             ADD_FAILURE() << "No matching timepoints for norm. Abort!";
         }
         else if(time_matching_norm[0][0]==-1)
         {
             ADD_FAILURE() << "No matching timepoints for norm. Abort!";
         }
         else
         {
             for(auto index_elem:time_matching_norm)
             {
                  EXPECT_NEAR(norm_cpp[index_elem[0]],norm_py[index_elem[1]],abstol);
             }
         }

    }
    else
    {
        ADD_FAILURE() << "Norm data missing cannot compare. Abort!";
    }
}

int main(int argc,char* argv[])
{
    global_argc=argc;
    global_argv=argv;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

