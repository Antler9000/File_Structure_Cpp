#include <iostream>
#include <fstream>
#include <random>
#include <cstring>
#include <vector>
#include <ctime>
#include <cmath>

using namespace std;
#define JoinCondition 2
#define BlockSize 4096
#define BiggestNum 100000000

bool    insertProfessor(string, unsigned, string);
bool    insertStudent(string, unsigned, float, unsigned);
bool    readStudent(unsigned, unsigned*, string*, float*, unsigned*);
bool    searchStudent(unsigned, string *, float*, unsigned*);
bool    join(string*, string*, unsigned);



class Block {
private:
    char data[BlockSize] = { 0, };
    unsigned index_of_data = 0;

public:
    Block() {}
    
    unsigned get_index() {
        return index_of_data;
    }

    inline bool sequential_write(char in_data) {
        if (index_of_data >= BlockSize) {
            cerr << "(class : block) out of index BlockSize while sequentail_write(..)" << endl;
            return false;
        }
        data[index_of_data] = in_data;
        index_of_data++;
        return true;
    }

    inline bool sequential_read(unsigned char& out_data) {
        if (index_of_data >= BlockSize) {
            cerr << "(class : block) out of index BlockSize while sequential_read(..)" << endl;
            return false;
        }
        out_data = data[index_of_data];
        index_of_data++;
        return true;
    }

    bool save_block(string file_name) {
        ofstream file(file_name, ios::binary | ios::app);
        if (!file) {
            cerr << "(class : block) could not open the file while save_block(..)" << file_name << endl;
            return false;
        }
        file.write(data, BlockSize);
        for (int i = 0; i < BlockSize; i++) {
            data[i] = 0;
        }
        index_of_data = 0;
        file.close();
        
        return true;
    }

    bool load_block(string file_name, unsigned index) {
        ifstream file(file_name, ios::binary);
        if (!file) {
            cerr << "(class : block) could not open the file while load_block(..)" << file_name << endl;
            return false;
        }
        file.seekg(index, ios::beg);
        file.read(data, BlockSize);
        index_of_data = 0;
        file.close();
        return true;
    }

    void copy_from(const Block& source_block) {       //index_of_data의 처리가 지금은 (read_buffer <- read_buffer) 방향으로의 복사에만 맞춰져있다는 점이 흠이다.
        for (int i = 0; i < 4096; i++) {
            data[i] = source_block.data[i];
        }
        index_of_data = 0;
    }
};



class File {
protected :
    string file_name;
    Block& write_buffer;    //쓰기 버퍼는 professor_file과 student_file이 공유하기에, 레퍼런스 변수로 뒀다
    Block read_buffer;
    size_t bytes_per_record;
    size_t records_per_block;
    unsigned number_of_record_written = 0;

    File(string file_name, Block& write_buffer, size_t bytes_per_record) : 
        file_name(file_name), write_buffer(write_buffer), bytes_per_record(bytes_per_record) 
    {
        records_per_block = BlockSize / bytes_per_record;
        initialize_file();
    }

    bool initialize_file() {
        ofstream file(file_name, ios::binary);
        if (!file) {
            cerr << "(class : File) could not open file while initialize_file()" << endl;
            return false;
        }
        file.close();
        return true;
    }

public :
    void finish_writing() {
        write_buffer.save_block(file_name);
    }

    unsigned get_number_of_record() {
        return number_of_record_written;
    }
};



class ProfessorFile : public File {
public :
    ProfessorFile(Block& write_buffer) : File("professor.dat", write_buffer, 34) {
        initialize_file();
    }

    bool write_record(string name, unsigned ID, string dept) {
        if (is_ID_duplicated(ID)) {
            return false;
        }

        if (BlockSize < write_buffer.get_index() + bytes_per_record) {
            write_buffer.save_block(file_name);
        }

        for (int i = 0; i < 20; i++) {
            if (name.size() <= i) write_buffer.sequential_write(0);
            else write_buffer.sequential_write(name[i]);
        }

        char* ID_string = reinterpret_cast<char*>(&ID);
        for (int i = 3; i >= 0; i--) {
            write_buffer.sequential_write(ID_string[i]);
        }

        for (int i = 0; i < 10; i++) {
            if (dept.size() <= i) write_buffer.sequential_write(0);
            else write_buffer.sequential_write(dept[i]);
        }

        number_of_record_written++;
        return true;
    }

    bool is_ID_duplicated(unsigned ID) {
        string dummy_name;
        string dummy_dept;
        if (search_record_by_ID(dummy_name, ID, dummy_dept) == true) {
            return true;
        }
        else {
            return false;
        }
    }

    bool search_record_by_ID(string& name, unsigned target_ID, string& dept) {
        for (int i = 0; i < number_of_record_written; i++) {
            if ((i % records_per_block) == 0) {
                unsigned index_of_block = i / records_per_block;
                if (number_of_record_written < (index_of_block + 1) * records_per_block) {
                    read_buffer.copy_from(write_buffer);
                }
                else {
                    read_buffer.load_block(file_name, index_of_block * BlockSize);
                }
            }

            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (사실 여기서 record_name은 과제 기능이 아니라 디버깅 목적으로만 사용된다)
            char record_name[20] = { 0, };
            for (int j = 0; j < 20; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_name[j] = out_data;
            }

            unsigned record_ID = 0;
            for (int j = 0; j < 4; j++) {
                //(주의) 
                //signed char은 산수 연산 때 4바이트 정수로 확장되며 부호가 확장되므로 예상치 못한 오류를 발생시킬 수 있다.
                //signed char의 msb가 1인 경우 이것이 부호확장되어 음수가 되어, 그 결과로 원래 구해져야하는 수에서 -256된 수가 조회되는 오류가 발생한다.
                //따라서 char을 다른 정수들에 더하는 등의 연산을 할 거면 이처럼 unsigned char로 선언하자.
                //(참고)
                //그 외의 경우엔 굳이 unsigned char로 선언할 필요는 없다. 
                //오히려 string으로의 암묵적 타입 변환을 할 때, const char*이 아닌 unsigned char*은 string으로 자동 변환이 되지 않으니 불편하기만 하다.
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_ID <<= 8;
                record_ID += out_data;
            }                           

            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (사실 여기서 record_dept는 과제 기능이 아니라 디버깅 목적으로만 사용된다)
            char record_dept[10] = { 0, };
            for (int j = 0; j < 10; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_dept[j] = out_data;
            }
            
            if (record_ID == target_ID) {
                name = record_name;
                dept = record_dept;
                cerr << "ID found (already inserted) : " << record_ID << " / name : " << record_name << " / dept : " << record_dept << endl;
                return true;
            }
            
        }
        return false;
    }

    bool search_record_by_rank(unsigned rank, string& name, unsigned& ID, string& dept) {
        if (number_of_record_written < (rank + 1)) {
            cerr << "rank is out of index" << endl;
            return false;
        }

        unsigned index_of_block = rank / records_per_block;
        if (number_of_record_written < (index_of_block + 1) * records_per_block) {
            read_buffer.copy_from(write_buffer);
        }
        else {
            read_buffer.load_block(file_name, index_of_block * BlockSize);
        }


        for (int i = 0; i < records_per_block; i++) {
            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (이 부분은 과제의 (2)번 기능을 위해 사용된다)
            char record_name[20] = { 0, };
            for (int j = 0; j < 20; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_name[j] = out_data;
            }

            unsigned record_ID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_ID <<= 8;
                record_ID += out_data;
            }

            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (사실 여기서 record_dept는 과제 기능이 아니라 디버깅 목적으로만 사용된다)
            char record_dept[10] = { 0, };
            for (int j = 0; j < 10; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_dept[j] = out_data;
            }

            if ((i + records_per_block * index_of_block) == rank) {
                name = record_name;
                ID = record_ID;
                dept = record_dept;
                return true;
            }
        }

        cerr << "maybe i skipped the rank you asked. my fault." << endl;
        return false;
    }
};



class StudentFile : public File {
public:
    StudentFile(Block& write_buffer) : File("student.dat", write_buffer, 32) {
        initialize_file();
    }

    bool write_record(string name, unsigned ID, float score, unsigned advisorID) {
        if (is_ID_duplicated(ID)) {
            return false;
        }

        if (BlockSize < write_buffer.get_index() + bytes_per_record) {
            write_buffer.save_block("student.dat");
        }

        for (int i = 0; i < 20; i++) {
            if (name.size() <= i) write_buffer.sequential_write(0);
            else write_buffer.sequential_write(name[i]);
        }

        char* ID_string = reinterpret_cast<char*>(&ID);
        for (int i = 3; i >= 0; i--) {
            write_buffer.sequential_write(ID_string[i]);
        }

        char* score_string = reinterpret_cast<char*>(&score);
        for (int i = 3; i >= 0; i--) {
            write_buffer.sequential_write(score_string[i]);
        }

        char* advisorID_string = reinterpret_cast<char*>(&advisorID);
        for (int i = 3; i >= 0; i--) {
            write_buffer.sequential_write(advisorID_string[i]);
        }

        number_of_record_written++;
        return true;
    }

    bool is_ID_duplicated(unsigned ID) {
        string dummy_name;
        float dummy_score;
        unsigned dummy_advisorID;
        if (search_record_by_ID(dummy_name, ID, dummy_score, dummy_advisorID) == true) {
            return true;
        }
        else {
            return false;
        }
    }

    bool search_record_by_ID(string& name, unsigned target_ID, float& score, unsigned& advisor_ID) {
        for (int i = 0; i < number_of_record_written; i++) {
            if ((i % records_per_block) == 0) {
                unsigned index_of_block = i / records_per_block;
                if (number_of_record_written < (index_of_block + 1) * records_per_block) {
                    read_buffer.copy_from(write_buffer);
                }
                else {
                    read_buffer.load_block(file_name, index_of_block * BlockSize);
                }
            }

            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (이 부분은 과제의 (3)번 기능을 위해 사용된다)
            char record_name[20] = { 0, };
            for (int j = 0; j < 20; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_name[j] = out_data;
            }

            unsigned record_ID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_ID <<= 8;
                record_ID += out_data;
            }

            unsigned record_score_bit_seq = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_score_bit_seq <<= 8;
                record_score_bit_seq += out_data;
            }
            float record_score = 0.0;
            memcpy(&record_score, &record_score_bit_seq, 4);

            unsigned record_advisorID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_advisorID <<= 8;
                record_advisorID += out_data;
            }

            if (record_ID == target_ID) {
                name = record_name;
                score = record_score;
                advisor_ID = record_advisorID;
                cout << "ID found (already inserted) : " << record_ID << " / name : " << record_name << " / score : " << record_score << " / advisor_ID : " << record_advisorID << endl;
                return true;
            }
        }
        return false;
    }

    bool search_record_by_rank(unsigned rank, string& name, unsigned& ID, float& score, unsigned& advisor_ID) {
        if (number_of_record_written < (rank + 1)) {
            cerr << "rank is out of index" << endl;
            return false;
        }

        unsigned index_of_block = rank / records_per_block;
        if (number_of_record_written < (index_of_block + 1) * records_per_block) {
            read_buffer.copy_from(write_buffer);
        }
        else {
            read_buffer.load_block(file_name, index_of_block * BlockSize);
        }


        for (int i = 0; i < records_per_block; i++) {
            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (이 부분은 과제의 (2)번 기능을 위해 사용된다)
            char record_name[20] = { 0, };
            for (int j = 0; j < 20; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_name[j] = out_data;
            }

            unsigned record_ID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_ID <<= 8;
                record_ID += out_data;
            }

            unsigned record_score_bit_seq = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_score_bit_seq <<= 8;
                record_score_bit_seq += out_data;
            }
            float record_score = 0.0;
            memcpy(&record_score, &record_score_bit_seq, 4);

            unsigned record_advisorID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_advisorID <<= 8;
                record_advisorID += out_data;
            }

            if ((i + records_per_block * index_of_block) == rank) {
                name = record_name;
                ID = record_ID;
                score = record_score;
                advisor_ID = record_advisorID;
                return true;
            }
        }
        
        cerr << "maybe i skipped the rank you asked. my fault." << endl;
        return false;
    }

    void sequential_search(const unsigned professor_ID, int& number_of_advised_student, unsigned& temporal_candidate_student_ID, string& temporal_candidate_student_name) {
        for (int i = 0; i < number_of_record_written; i++) {
            if ((i % records_per_block) == 0) {
                unsigned index_of_block = i / records_per_block;
                if (number_of_record_written < (index_of_block + 1) * records_per_block) {
                    read_buffer.copy_from(write_buffer);
                }
                else {
                    read_buffer.load_block(file_name, index_of_block * BlockSize);
                }
            }

            //string 대용의 로컬 어레이 사용은 용인하도록 하자. (이 부분은 과제의 (4)번 기능을 위해 사용된다)
            char record_name[20] = { 0, };
            for (int j = 0; j < 20; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_name[j] = out_data;
            }

            unsigned record_ID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_ID <<= 8;
                record_ID += out_data;
            }

            unsigned record_score_bit_seq = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_score_bit_seq <<= 8;
                record_score_bit_seq += out_data;
            }
            float record_score = 0.0;
            memcpy(&record_score, &record_score_bit_seq, 4);

            unsigned record_advisorID = 0;
            for (int j = 0; j < 4; j++) {
                unsigned char out_data = 0;
                read_buffer.sequential_read(out_data);
                record_advisorID <<= 8;
                record_advisorID += out_data;
            }

            if (professor_ID == record_advisorID) {
                number_of_advised_student++;
                if (record_ID < temporal_candidate_student_ID) {
                    temporal_candidate_student_ID = record_ID;
                    temporal_candidate_student_name = record_name;
                }
            }

        }
    }
};



Block shared_write_buffer = Block();
ProfessorFile professor_file = ProfessorFile(shared_write_buffer);
StudentFile student_file = StudentFile(shared_write_buffer);



int main() {
    unsigned        numberStudents, numberProfessors, numberID, numberRank;
    clock_t         t_start, t_end;

    ifstream        prof_ifs("professor.txt", ios::in);
    ifstream        stud_ifs("student.txt", ios::in);
    ifstream        rank_ifs("rankQuery.txt", ios::in);
    ifstream        ID_ifs("IDQuery.txt", ios::in);

    if(prof_ifs.fail()) {
        cerr<<"Professor.txt Open Error"<<endl;
        exit(1);
    }

    if(stud_ifs.fail()) {
        cerr<<"Student.txt Open Error"<<endl;
        exit(1);
    }

    if(rank_ifs.fail()) {
        cerr<<"rank_query.txt Open Error"<<endl;
        exit(1);
    }


    if(ID_ifs.fail()) {
        cerr<<"IDQuery.txt Open Error"<<endl;
        exit(1);
    }


    string      name;
    string      dept;
    unsigned    ID;
    float       score;
    unsigned    advisorID;
    int         n;
    string      pname, sname;


    prof_ifs>>numberProfessors;
    stud_ifs>>numberStudents;
    rank_ifs>>numberRank;
    ID_ifs>>numberID;
    
    t_start=clock();

    //교수 = name 20바이트 + ID 4바이트 + dept 10바이트 = 총 34바이트
    //한 블럭 내 교수 수 : 120개 (0~119)
    //학생 = ID 4바이트 + name 20바이트 + score 4바이트 + advisorID 4바이트 = 총 32바이트
    //한 블럭 내 학생 수 : 128개 (0~127)
    //unsigned number_of_professor_blocks = (unsigned) ceil((float) numberProfessors / 120);
    //unsigned number_of_student_blocks = (unsigned)ceil((float)numberStudents / 128);
    //cout << "number_of_professor_blocks (1-based) " << "\t : " << number_of_professor_blocks << endl;
    //cout << "number_of_student_blocks (1-based) " << "\t : " << number_of_student_blocks << endl;
    
    //예상 구간 소요 시간 20초
    cout << "--------------phase 1/5--------------" << endl;
    for(unsigned i=0;i<numberProfessors;i++) {
        prof_ifs>>name>>ID>>dept;
        if(insertProfessor(name,ID,dept)!=true) {
            cerr<<"Professor Insert Error. Doing next job..."<<endl;
            //exit(1);
        }
    }
    professor_file.finish_writing();

    //예상 구간 소요 시간 85초
    cout << "--------------phase 2/5--------------" << endl;
    for(unsigned i=0;i<numberStudents;i++) {
        stud_ifs>>name>>ID>>score>>advisorID;
        if(insertStudent(name,ID,score,advisorID)!=true) {
            cerr<<"Student Insert Error. Doing next job..."<<endl;
            //exit(1);
        }
    }
    student_file.finish_writing();

    //예상 구간 소요 시간 5초
    cout << "--------------phase 3/5--------------" << endl;
    for(unsigned i=0;i<numberRank;i++) {
        rank_ifs>>n;
        if(readStudent(n,&ID, &name,&score,&advisorID)!=true) {
            cerr<<"Student Read Error. Doing next job..."<<endl;
            //exit(1);
        }
    }

    //예상 구간 소요 시간 5초
    cout << "--------------phase 4/5--------------" << endl;
    for(unsigned i=0;i<numberID;i++) {
        ID_ifs>>ID;
        if(searchStudent(ID,&name,&score,&advisorID)!=true) {
            cerr<<"Student Search Error. Doing next job..."<<endl;
            //exit(1);
        }
    }
    
    //예상 구간 소요 시간 100초
    cout << "--------------phase 5/5--------------" << endl;
    if(join(&pname, &sname, JoinCondition)!=true) {
            cerr<<"Join Error. Doing next job..."<<endl;
            //exit(1);
    }

    t_end=clock();

    prof_ifs.close();
    stud_ifs.close();
    rank_ifs.close();
    ID_ifs.close();

    double duration=(double)(t_end-t_start)/CLOCKS_PER_SEC;
    cout<<"Execution time: "<<duration<<endl;

    return 0;
}



bool    insertProfessor(string a, unsigned b, string c) { 
    if (professor_file.write_record(a, b, c) == false) {
        cerr << "ID duplicated (wanna insert) : " << b << " / name : " << a << " / dept : " << c << endl;
        return false;
    }
    else {
        return true;
    }
}



bool    insertStudent(string a, unsigned b, float c, unsigned d) { 
    if (student_file.write_record(a, b, c, d) == false) {
        cerr << "ID duplicated (wanna insert) : " << b << " / name : " << a << " / score : " << c << " / advisor_ID : " << d << endl;
        return false;
    }
    else {
        return true;
    }
}



bool    searchStudent(unsigned ID, string* name, float* score, unsigned* advisorID){
    if (student_file.search_record_by_ID(*name, ID, *score, *advisorID) == false) {
        cerr << "could not found such ID student (ID : " << ID << " )" << endl;
        return false;
    }
    else {
        return true;
    }
}



bool    readStudent(unsigned n, unsigned* ID, string* name, float* score, unsigned* advisorID) {
    if (student_file.search_record_by_rank(n, *name, *ID, *score, *advisorID) == false) {
        cerr << "could not found such rank student (rank : " << n << " )" << endl;
        return false;
    }
    else {
        cout << "successful student_search_by_rank : (ID = " << *ID << ")" << endl;
        return true;
    }
}



bool    join(string* pname, string* sname, unsigned n) {
    bool found = false;

    unsigned candidate_professor_ID = BiggestNum;
    string candidate_professor_name;
    unsigned candidate_student_ID = BiggestNum;
    string candidate_student_name;

    for (int i = 0; i < professor_file.get_number_of_record(); i++) {
        string professor_name;
        unsigned professor_ID;
        string professor_dept;

        professor_file.search_record_by_rank(i, professor_name, professor_ID, professor_dept);
        
        string student_name;
        unsigned student_ID;
        float student_score;
        unsigned student_advisorID;

        int number_of_advised_student = 0;
        unsigned temporal_candidate_student_ID = BiggestNum;
        string temporal_candidate_student_name;

        student_file.sequential_search(professor_ID, number_of_advised_student, temporal_candidate_student_ID, temporal_candidate_student_name);

        if (number_of_advised_student == n) {
            found = true;
            if (professor_ID < candidate_professor_ID) {
                candidate_professor_ID = professor_ID;
                candidate_professor_name = professor_name;
                if (temporal_candidate_student_ID < candidate_student_ID) {
                    candidate_student_ID = temporal_candidate_student_ID;
                    candidate_student_name = temporal_candidate_student_name;
                }
            }
        }
    }

    if (found == true) {
        cout << "found the join! professor_ID = " << candidate_professor_ID << ", student_ID = " << candidate_student_ID << endl;
        *pname = candidate_professor_name;
        *sname = candidate_student_name;
        return true;
    }
    else {
        return false;
    }
}