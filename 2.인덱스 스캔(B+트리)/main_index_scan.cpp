#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>

using namespace std;
/*---------------------------------------------------------------*/
/*--------------------------미완---------------------------------*/
//[1]삽입 메소드에서 작성 못한 것
//1.파일에서 구조체를 읽고 쓰는 입출력 부분
//2.삽입 메소드에서 꽉찬 내부 노드의 분할 로직
//3.삽입 메소드에서 데이터를 삽입할 리프 노드를 찾아 탐색해가는 로직
// 
//[2]검색 메소드에서 작성 못한 것
//1.전체 다
//
//[3]디버깅 한 적 없음
/*---------------------------------------------------------------*/

//한 학생의 (점수, 레코드가 저장된 student.dat 파일에서의 블락 번호) 쌍을 하나의 데이터로서 B+트리에 삽입되고 검색되고 삭제될 것임
//추정 크기 = 8 바이트
struct BPlusTreeData {
    float score;
    unsigned block_index;
};

//B+트리의 특성상, 노드들을 너비우선탐색 순서로 인덱스 파일에 쓰는 것은 부적합하다. 
//왜냐하면 B+트리에서 새로운 노드는 형제자매나 부모로 만들어지기에, 높이를 기준으로 쓰인 파일에 이들을 새로 쓰기 위해선 기존의 내용을 미뤄써야하기 때문이다.
//따라서, 마치 동적할당처럼 부모 형제 자식 관계에 상관 없이 노드를 쓰되, 각 노드에 인덱스를 부여하고서 이들과 실제 저장 블럭 번호를 매핑해주는 슈퍼블럭을 두자.
//슈퍼블럭은 인덱스 파일의 첫 블럭에 두고서, 헤드 노드의 인덱스가 무엇인지와, 그리고 각 노드 인덱스와 그것의 저장 블럭 번호와 리프 노드 여부를 써놓자.
//추정 크기 = 12 바이트 : 4 바이트(unsigned) + 4 바이트(unsigned) + 1 바이트(bool) + 3 바이트(padding)
struct SuperBlockNodeInfo {
    unsigned node_index;
    unsigned block_index;
    bool is_leaf_node;
};

//추정 크기 = 4092 바이트 = 4 바이트(unsigned) + 4 바이트(unsigned) + 4 바이트(padding) + (12 바이트(SuperBlockMappingData) * 340)
//참고 : VS Studio에서의 크기는 패딩이 제외되어 4084 바이트
struct BPlusTreeSuperBlock {
    unsigned head_node_block_index;
    unsigned number_of_mapping_data;
    SuperBlockNodeInfo mapping_data_array[340];
};

//점수 키들의 최대 개수는 511
//내부 노드는 데이터를 저장하지 않고 탐색을 위한 키만을 저장한다. 데이터는 리프 노드에 저장돼 있다.
//자기 자신을 포함해서 자신과 같은 키 값을 가진 데이터 중 일부가 왼쪽 하위 트리의 리프 노드에 있으므로,
//탐색하고자 하는 키가 내부 노드의 키와 같으면 왼쪽 자식으로 탐색해나가야 한다. 
//이후 리프에 도착해서 우측 방향 연결 리스트를 따라 탐색하면 실제 데이터를 얻을 수 있다.
//n번째 데이터의 왼쪽 자식을 참조하기 위한 인덱스는 n, 오른쪽 자식을 참조하기 위한 인덱스는 n+1이다.
//추정 크기 : 4092 바이트 = (4 바이트(float) * 511) + (4 바이트(unsigned) * 512)
struct BPlusTreeInnerDataBlock {
    float score_array[511];
    unsigned child_node_index_array[511 + 1];
};

//최대 데이터 개수는 511
//B+트리에서 리프 노드는 구조 상 오른쪽 리프 노드와 연결되어야 하므로 추가 노드 인덱스를 둬야 한다.
//추정 크기 : 4096 바이트 : (8 바이트(BPlusTreeData) * 511) + 4 바이트(unsigned) + 4 바이트(padding)
//참고 : VS Studio에서의 크기는 패딩이 제외되어 4092 바이트
struct BPlusTreeLeafDataBlock {
    BPlusTreeData data_array[511];
    unsigned next_leaf_node_index;
};

//B+트리의 삽입 메소드에서 리프 노프에서의 분할은 루트 노드까지 거슬러 올라 올 수 있다. 거슬러 올라 오려면 경로를 기억해야 하므로 스택을 정의하자.
class node_index_stack;

class node_index_stack_data_node {
    friend node_index_stack;

    unsigned node_index;
    node_index_stack_data_node* next_ptr;

    node_index_stack_data_node(unsigned new_node_index, node_index_stack_data_node* new_next_ptr) : node_index(new_node_index), next_ptr(new_next_ptr) {}
};

class node_index_stack {
    node_index_stack_data_node* head_node;
    unsigned item_number;

public:
    node_index_stack() : head_node(NULL), item_number(0) {}

    void push(unsigned new_node_index) {
        if (head_node == NULL) {
            head_node = new node_index_stack_data_node(new_node_index, NULL);
            item_number = 1;
            return;
        }

        node_index_stack_data_node* search_ptr = head_node;
        for (int i = 0; i < item_number - 1; i++) {
            search_ptr = search_ptr->next_ptr;
        }

        search_ptr->next_ptr = new node_index_stack_data_node(new_node_index, NULL);
        item_number++;
    }

    unsigned pop() {
        if (head_node == NULL) return 0;

        node_index_stack_data_node* search_ptr = head_node;
        for (int i = 0; i < item_number - 2; i++) {
            search_ptr = search_ptr->next_ptr;
        }
        unsigned return_node_index = search_ptr->next_ptr->node_index;
        delete search_ptr->next_ptr;
        search_ptr->next_ptr = NULL;
        item_number--;
        return return_node_index;

    }
};

//학생의 점수(float)을 기준으로 한 B+트리 클래스
//삭제는 과제에서 필요하지 않는 기능이라 일단은 생략.
class StudentScoreBPlusTree {
    string index_file_name = "student.idx";

public:
    void insertion(float score, unsigned block_index) {
        BPlusTreeData new_data = { score, block_index };

        //헤드노드의 인덱스를 알아내어 탐색 인덱스의 첫 값으로 대입하자. 또한 해당 노드가 리프 노드인지 여부와 현재 트리에 존재하는 노드 개수도 읽어내자.
        BPlusTreeSuperBlock* super_block = new BPlusTreeSuperBlock;
        super_block;                    //TODO : 파일 입출력
        unsigned search_node_index = 0; //TODO : 파일 입출력
        unsigned number_of_nodes = 0;   //TODO : 파일 입출력
        bool is_leaf_node = 0;          //TODO : 파일 입출력
        delete super_block;
        super_block = NULL;

        //빈 트리인 경우 첫 리프 노드를 생성하자.
        if (search_node_index == NULL) {
            BPlusTreeLeafDataBlock* new_leaf_data_block = new BPlusTreeLeafDataBlock{ { 0, }, NULL };
            new_leaf_data_block->data_array[0] = new_data;
            //인덱스 파일에다가 새 리프 블럭을 써주자.
            //TODO : 파일 입출력
            delete new_leaf_data_block;
            new_leaf_data_block = NULL;
            super_block = new BPlusTreeSuperBlock;
            //인덱스 파일에다가 슈퍼 블럭에 새로 생긴 노드의 정보를 업데이트 해주자.
            //TODO : 파일 입출력
            delete super_block;
            super_block = NULL;
            return;
        }

        //빈 트리가 아니면 트리를 탐색하여 리프노드까지 내려가자. 거치는 경로상의 노드 인덱스들은 스택에다가 저장하자.
        node_index_stack search_route_stack = node_index_stack();
        while (search_node_index != NULL) {
            //리프 노드이면 삽입을 수행하자.
            if (is_leaf_node == true) {
                //리프 노드를 파일로부터 읽어오자.
                BPlusTreeLeafDataBlock* leaf_data_block = new BPlusTreeLeafDataBlock{ {0, }, NULL };
                //TODO : 파일 입출력

                //리프 노드 내 데이터의 개수를 알아내자.
                unsigned data_count = 0;
                while (leaf_data_block->data_array[data_count].block_index != NULL) data_count++;

                //데이터를 하나 더 넣을 여유 공간이 있는지 확인하자.
                if (data_count < 511) {
                    //넣을 수 있으면, 이미 오름차순으로 정렬되어 있는 (0~data-count-1) 요소들에 오름차순을 지켜 삽입하기
                    unsigned insertion_index = 0;
                    while (leaf_data_block->data_array[insertion_index].score < new_data.score) insertion_index++;
                    //insertion_index의 데이터의 점수는 새로운 데이터의 점수보다 크거나 같다. 이것부터 data_count - 1 인덱스까지의 데이터를 한 칸씩 미루고, insertion_index의 위치에 new_data를 삽입하자.
                    for (int i = data_count - 1; insertion_index <= i; i--) {
                        leaf_data_block->data_array[i + 1] = leaf_data_block->data_array[i];
                    }
                    leaf_data_block->data_array[insertion_index] = new_data;

                    //새로운 노드를 생성한 것은 아니니 슈퍼블럭을 업데이트 할 필요는 없다.
                    //업데이트 된 리프 데이터 블럭을 써주는 것으로 끝내자.
                    //TODO : 파일 입출력
                    return;
                }
                //꽉차있다면 노드의 데이터를 반으로 분할하자. 이때
                //부모 내부 노드도 꽉 찼다면 분할을 수행해나가자. 실행 시간 상 최악의 경우엔, 루트 노드를 분할 하는 것까지 거슬러 올라간다.
                else {
                    //우선 삽입할 데이터들을 포함해서 오름 차순으로 정렬하자. 그렇게 해야 이후 분할을 할 수 있을 것이다.
                    //일단 삽입할 위치를 찾자.
                    unsigned insertion_index = 0;
                    while (leaf_data_block->data_array[insertion_index].score < new_data.score) insertion_index++;

                    //insertion_index의 데이터의 점수는 새로운 데이터의 점수보다 크거나 같다. 우선 끝(510 인덱스)의 요소를 따로 변수에 저장하고, insertion_index부터 끝이전까지(509 인덱스까지)의 데이터를 한 칸씩 미루자.
                    //그리고 insertion_index 위치에 새 데이터를 삽입해서 오름차순을 완성하자.
                    //TODO : 삽입 키가 가장 큰 경우 처리
                    BPlusTreeData last_data = leaf_data_block->data_array[510];
                    for (int i = 510 - 1; insertion_index <= i; i--) {
                        leaf_data_block->data_array[i + 1] = leaf_data_block->data_array[i];
                    }
                    leaf_data_block->data_array[insertion_index] = new_data;

                    //리프노드에 이미 꽉차 있는 데이터는 511개, 여기에 새로운 데이터까지 하나 합하면 512개이다.
                    //그렇다면 256개씩 두 개의 리프 노드로 분할하면 된다. 여기서 부모로 올릴 분할 키는 왼쪽 분할 노드의 가장 큰 키 값이다.
                    BPlusTreeLeafDataBlock* second_leaf_data_block = new BPlusTreeLeafDataBlock{ {0, }, NULL };
                    for (int i = 0; i < 255; i++) {
                        second_leaf_data_block->data_array[i] = leaf_data_block->data_array[256 + i];
                        leaf_data_block->data_array[256 + i] = BPlusTreeData{ 0, 0 };
                    }
                    second_leaf_data_block->data_array[255] = last_data;

                    float division_key = leaf_data_block->data_array[255].score;
          
                    //업데이트 된 leaf_data_block과 분할되며 새로 만들어진 seconde_data_block을 쓰자. 그리고 슈퍼 블락에도 seconde_data_block의 정보를 업데이트 하자. 새로 생긴 seconde_data_block의 노드 인덱스는 number_of_nodes을 1 증가시킨 값으로 배정하자.
                    number_of_nodes++;
                    //TODO : 파일 입출력
                    delete leaf_data_block;
                    delete second_leaf_data_block;
                    leaf_data_block = NULL;
                    second_leaf_data_block = NULL;
                    
                    //분할을 통해 나온 중앙값 키를 부모로 올려야 한다. 최악의 경우 루트노드까지 거슬러 올라가 분할해야 한다.
                    unsigned division_node_index = search_route_stack.pop();

                    while (division_node_index != NULL) {
                        //부모 노드가 있으면 거기에 넣을 수 있는지 확인해보자.
                        if (division_node_index != NULL) {
                            //부모의 노드 인덱스를 통해 해당 내부 노드를 불러오자.
                            BPlusTreeInnerDataBlock* division_node = new BPlusTreeInnerDataBlock{ {0,}, {0,} };
                            //TODO : 파일 입출력

                            //해당 노드에 넣을 수 있는 키 여유 공간이 1개 있는지 확인하자. inner node의 점수 키 값들은 -1.0으로 초기화되어 있으니 0 이상인지를 확인하면 된다.
                            unsigned inner_key_count = 0;
                            while (division_node->score_array[inner_key_count] >= 0.0) inner_key_count++;

                            //여유 공간이 있으면 division_key를 오름 차순에 맞게 삽입하고, 새로운 second_leaf_data_block의 노드 인덱스도 자식 포인터로서 순서에 맞게 삽입하고서 끝내면 된다.
                            if (inner_key_count < 511) {
                                //TODO : 어레이에 데이터를 오름차순에 맞게 삽입(키 어레이, 자식 노드 인덱스 어레이)
                                return;
                            }
                            //여유 공간이 없다면, 이 내부 노드도 분할하고서, 부모를 향해 키 삽입이 거슬러 올라간다.
                            else {
                                //우선 삽입할 점수 키를 포함해서 오름 차순으로 정렬하자. 그렇게 해야 이후 분할을 할 수 있을 것이다.
                                //일단 삽입할 위치를 찾자.
                                unsigned inner_insertion_index = 0;
                                while (division_node->score_array[inner_insertion_index] < division_key) inner_insertion_index++;

                                //inner_insertion_index가 가리키는 점수 키는 자식의 분할로 올라온 division_key의 점수보다 크거나 같다. 우선 끝(510 인덱스)의 키를 따로 변수에 저장하고, inner_insertion_index부터 끝이전까지(509 인덱스까지)의 데이터를 한 칸씩 미루자.
                                //그리고 inner_insertion_index 위치에 새 데이터를 삽입해서 오름차순을 완성하자.
                                //자식 노드 인덱스 어레이들은, 분할로 생성된 새로운 노드의 인덱스를 삽입해야 하므로, inner_insertion_index+1의 인덱스에 위치한 자식 노드 인덱스(새로 삽입되는 키의 오른쪽 자식)부터 다 하나씩 밀고서, 해당 자리에다가 새로 생긴 노드를 삽입해주자.
                                //TODO : 삽입 키가 가장 큰 경우 처리, 아래 로직에서 자식 노드 인덱스 로직이 맞는지 한 번 더 확인하기.
                                float last_key = division_node->score_array[510];
                                unsigned last_child_node_index = division_node->child_node_index_array[511];

                                for (int i = 510 - 1; inner_insertion_index <= i; i--) {
                                    division_node->score_array[i + 1] = division_node->score_array[i];
                                    division_node->child_node_index_array[i + 2] = division_node->score_array[i + 1];
                                }
                                division_node->child_node_index_array[inner_insertion_index + 2] = division_node->child_node_index_array[inner_insertion_index + 1];

                                division_node->score_array[inner_insertion_index] = division_key;
                                division_node->child_node_index_array[inner_insertion_index+1] = number_of_nodes;


                                //오름차순으로 정렬된 키와 자식 노드 인덱스 어레이들을 반반으로 나눠서 새로운 내부 노드로 분할하기
                                //TODO : 새 내부 노드 생성해서 키와 자식 노드 인덱스의 반 대입하기
     

                                //업데이트 된 내부노드와, 새로 생성된 내부 노드를 파일에 적기. 새로 생성된 내부 노드의 인덱스는 number_of_nodes를 1 증가시킨 값으로 한다.
                                //TODO : 파일 입출력
                            }
                        }
                    }
                    //여기까지 왔다는 것은 루트노드에서 분할이 일어났음을 의미한다.
                    
                }
            }
            //리프 노드가 아니라면 탐색을 위해 내려갈 자식 노드 인덱스를 알아내어 search_index에 대입하자.
            //내부 노드의 키 스코어에 대응되는 실제 데이터는 중위선행자 리프노드에 위치한다. 또한 해당 키와 중복되는 키의 데이터들도 그곳에 위치한다.
            //따라서 찾고자 하는 데이터의 키가 내부 노드의 키와 일치하면 왼쪽 자식으로 내려가야 한다.
            //이때 이번에 있었던 노드의 인덱스를 탐색 경로를 저장하는 스택에다가 푸쉬 해두자. 분할이 루트를 향해 올라갈 때 필요하다.
            else {
                //TODO
                return;
            }
        }

    }

    //과제의 요구 사항 중, 탐색 점수 구간에 만족하는 학생 중 가장 먼저 삽입된 학생을 알아내기 위해서는,
    //삽입된 순서대로 student.dat 파일에 써져 있는 학생 중 가장 앞에 쓰인 학생을 알아내면 된다.
    //이를 알아내기 위해서 B+트리의 서치에서는 구간에 만족하는 학생들 각각의 student.dat 파일내 블럭 번호 중, 가장 최소 값을 알아내서 반환하자.
    //이를 호출한 함수에서는 해당 정보를 가지고서 student.dat 파일의 해당 블럭을 직접 확인하여, 
    //블럭 내에서 점수 구간을 만족하는 학생 중 가장 위에 쓰인 학생을 반환하면 될 것이다.
    unsigned search(float start, float end, unsigned& minimum_of_block_index) {

    }
};

class Student{
public:
    unsigned    ID;
    string      name;
    float       score;
    unsigned    advisorID;
};

bool insertStudent(string, unsigned, float, unsigned);

unsigned searchStudent(float minScore, float maxScore, Student* stud);


int main() {
    /*
    unsigned numberStudents, numberQueries;
    
    ifstream stud_ifs("student.txt", ios::in);
    ifstream query_ifs("query.txt", ios::in);
    ofstream result_ofs("result.txt", ios::out);

    if(stud_ifs.fail()) {
        cerr<<"Student.txt Open Error"<<endl;
        exit(1);
    }

    if(query_ifs.fail()) {
        cerr<<"Query.txt Open Error"<<endl;
        exit(1);
    }

    string name;
    unsigned ID;
    float score;
    unsigned advisorID;
    float minScore,maxScore;
    Studentstud;

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
    */
   return 0;
}

bool insertStudent(string name, unsigned ID, float score, unsigned advisorID) { 
    return true; 
}

//과제의 요구사항은 구간에 만족하는 학생의 수와, 그 중에 가장 먼저 삽입된 학생을 알아내는 것이다.
//수는 반환자를 통해서 전달하고, 가장 먼저 삽입된 학생은 출력 매개변수인 student에 담으면 된다.
unsigned searchStudent(float minimum_score, float max_score, Student* student) {
    student->ID=12345; student->name="abcd"; student->score=80.5; student->advisorID=654321;
    return 10;;
}
