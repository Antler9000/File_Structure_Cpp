#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>

using namespace std;

class Student{
public:
        unsigned    ID;
        string      name;
        float       score;
        unsigned    advisorID;
};

bool        insertStudent(string, unsigned, float, unsigned);
unsigned    searchStudent(float minScore, float maxScore, Student* stud);


int main() {

    unsigned        numberStudents, numberQueries;

    ifstream        stud_ifs("student.txt", ios::in);
    ifstream        query_ifs("query.txt", ios::in);
    ofstream        result_ofs("result.txt", ios::out);

    if(stud_ifs.fail()) {
        cerr<<"Student.txt Open Error"<<endl;
        exit(1);
    }

    if(query_ifs.fail()) {
        cerr<<"Query.txt Open Error"<<endl;
        exit(1);
    }

    string      name;
    unsigned    ID;
    float       score;
    unsigned    advisorID;
    float       minScore,maxScore;
    Student     stud;

    stud_ifs>>numberStudents;
    query_ifs>>numberQueries;

    for(unsigned i=0;i<numberStudents;i++) {
        stud_ifs>>name>>ID>>score>>advisorID;
        if(insertStudent(name,ID,score,advisorID)!=true)
            cerr<<"Student Insert Error"<<endl;
    }

    query_ifs>>numberQueries;
    unsigned    nStud;
    for(unsigned i=0;i<numberQueries;i++) {
        query_ifs>>minScore>>maxScore;
        nStud=searchStudent(minScore,maxScore, &stud);
        result_ofs<<nStud<<" "<<stud.ID<<endl;
    }

    stud_ifs.close();
    query_ifs.close();
    result_ofs.close();

   return 0;
}

// the functions below are dummy test functions. Replace them with your program
bool        insertStudent(string a, unsigned b, float c, unsigned d) { return true; }
unsigned    searchStudent(float a, float b, Student* s) {
    s->ID=12345; s->name="abcd"; s->score=80.5; s->advisorID=654321;
    return 10;;
}
