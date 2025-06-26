#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>

using namespace std;
/*---------------------------------------------------------------*/
/*--------------------------�̿�---------------------------------*/
//[1]���� �޼ҵ忡�� �ۼ� ���� ��
//1.���Ͽ��� ����ü�� �а� ���� ����� �κ�
//2.���� �޼ҵ忡�� ���� ���� ����� ���� ����
//3.���� �޼ҵ忡�� �����͸� ������ ���� ��带 ã�� Ž���ذ��� ����
// 
//[2]�˻� �޼ҵ忡�� �ۼ� ���� ��
//1.��ü ��
//
//[3]����� �� �� ����
/*---------------------------------------------------------------*/

//�� �л��� (����, ���ڵ尡 ����� student.dat ���Ͽ����� ��� ��ȣ) ���� �ϳ��� �����ͷμ� B+Ʈ���� ���Եǰ� �˻��ǰ� ������ ����
//���� ũ�� = 8 ����Ʈ
struct BPlusTreeData {
    float score;
    unsigned block_index;
};

//B+Ʈ���� Ư����, ������ �ʺ�켱Ž�� ������ �ε��� ���Ͽ� ���� ���� �������ϴ�. 
//�ֳ��ϸ� B+Ʈ������ ���ο� ���� �����ڸų� �θ�� ��������⿡, ���̸� �������� ���� ���Ͽ� �̵��� ���� ���� ���ؼ� ������ ������ �̷����ϱ� �����̴�.
//����, ��ġ �����Ҵ�ó�� �θ� ���� �ڽ� ���迡 ��� ���� ��带 ����, �� ��忡 �ε����� �ο��ϰ� �̵�� ���� ���� �� ��ȣ�� �������ִ� ���ۺ��� ����.
//���ۺ��� �ε��� ������ ù ���� �ΰ�, ��� ����� �ε����� ����������, �׸��� �� ��� �ε����� �װ��� ���� �� ��ȣ�� ���� ��� ���θ� �����.
//���� ũ�� = 12 ����Ʈ : 4 ����Ʈ(unsigned) + 4 ����Ʈ(unsigned) + 1 ����Ʈ(bool) + 3 ����Ʈ(padding)
struct SuperBlockNodeInfo {
    unsigned node_index;
    unsigned block_index;
    bool is_leaf_node;
};

//���� ũ�� = 4092 ����Ʈ = 4 ����Ʈ(unsigned) + 4 ����Ʈ(unsigned) + 4 ����Ʈ(padding) + (12 ����Ʈ(SuperBlockMappingData) * 340)
//���� : VS Studio������ ũ��� �е��� ���ܵǾ� 4084 ����Ʈ
struct BPlusTreeSuperBlock {
    unsigned head_node_block_index;
    unsigned number_of_mapping_data;
    SuperBlockNodeInfo mapping_data_array[340];
};

//���� Ű���� �ִ� ������ 511
//���� ���� �����͸� �������� �ʰ� Ž���� ���� Ű���� �����Ѵ�. �����ʹ� ���� ��忡 ����� �ִ�.
//�ڱ� �ڽ��� �����ؼ� �ڽŰ� ���� Ű ���� ���� ������ �� �Ϻΰ� ���� ���� Ʈ���� ���� ��忡 �����Ƿ�,
//Ž���ϰ��� �ϴ� Ű�� ���� ����� Ű�� ������ ���� �ڽ����� Ž���س����� �Ѵ�. 
//���� ������ �����ؼ� ���� ���� ���� ����Ʈ�� ���� Ž���ϸ� ���� �����͸� ���� �� �ִ�.
//n��° �������� ���� �ڽ��� �����ϱ� ���� �ε����� n, ������ �ڽ��� �����ϱ� ���� �ε����� n+1�̴�.
//���� ũ�� : 4092 ����Ʈ = (4 ����Ʈ(float) * 511) + (4 ����Ʈ(unsigned) * 512)
struct BPlusTreeInnerDataBlock {
    float score_array[511];
    unsigned child_node_index_array[511 + 1];
};

//�ִ� ������ ������ 511
//B+Ʈ������ ���� ���� ���� �� ������ ���� ���� ����Ǿ�� �ϹǷ� �߰� ��� �ε����� �־� �Ѵ�.
//���� ũ�� : 4096 ����Ʈ : (8 ����Ʈ(BPlusTreeData) * 511) + 4 ����Ʈ(unsigned) + 4 ����Ʈ(padding)
//���� : VS Studio������ ũ��� �е��� ���ܵǾ� 4092 ����Ʈ
struct BPlusTreeLeafDataBlock {
    BPlusTreeData data_array[511];
    unsigned next_leaf_node_index;
};

//B+Ʈ���� ���� �޼ҵ忡�� ���� ���������� ������ ��Ʈ ������ �Ž��� �ö� �� �� �ִ�. �Ž��� �ö� ������ ��θ� ����ؾ� �ϹǷ� ������ ��������.
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

//�л��� ����(float)�� �������� �� B+Ʈ�� Ŭ����
//������ �������� �ʿ����� �ʴ� ����̶� �ϴ��� ����.
class StudentScoreBPlusTree {
    string index_file_name = "student.idx";

public:
    void insertion(float score, unsigned block_index) {
        BPlusTreeData new_data = { score, block_index };

        //������� �ε����� �˾Ƴ��� Ž�� �ε����� ù ������ ��������. ���� �ش� ��尡 ���� ������� ���ο� ���� Ʈ���� �����ϴ� ��� ������ �о��.
        BPlusTreeSuperBlock* super_block = new BPlusTreeSuperBlock;
        super_block;                    //TODO : ���� �����
        unsigned search_node_index = 0; //TODO : ���� �����
        unsigned number_of_nodes = 0;   //TODO : ���� �����
        bool is_leaf_node = 0;          //TODO : ���� �����
        delete super_block;
        super_block = NULL;

        //�� Ʈ���� ��� ù ���� ��带 ��������.
        if (search_node_index == NULL) {
            BPlusTreeLeafDataBlock* new_leaf_data_block = new BPlusTreeLeafDataBlock{ { 0, }, NULL };
            new_leaf_data_block->data_array[0] = new_data;
            //�ε��� ���Ͽ��ٰ� �� ���� ���� ������.
            //TODO : ���� �����
            delete new_leaf_data_block;
            new_leaf_data_block = NULL;
            super_block = new BPlusTreeSuperBlock;
            //�ε��� ���Ͽ��ٰ� ���� ���� ���� ���� ����� ������ ������Ʈ ������.
            //TODO : ���� �����
            delete super_block;
            super_block = NULL;
            return;
        }

        //�� Ʈ���� �ƴϸ� Ʈ���� Ž���Ͽ� ���������� ��������. ��ġ�� ��λ��� ��� �ε������� ���ÿ��ٰ� ��������.
        node_index_stack search_route_stack = node_index_stack();
        while (search_node_index != NULL) {
            //���� ����̸� ������ ��������.
            if (is_leaf_node == true) {
                //���� ��带 ���Ϸκ��� �о����.
                BPlusTreeLeafDataBlock* leaf_data_block = new BPlusTreeLeafDataBlock{ {0, }, NULL };
                //TODO : ���� �����

                //���� ��� �� �������� ������ �˾Ƴ���.
                unsigned data_count = 0;
                while (leaf_data_block->data_array[data_count].block_index != NULL) data_count++;

                //�����͸� �ϳ� �� ���� ���� ������ �ִ��� Ȯ������.
                if (data_count < 511) {
                    //���� �� ������, �̹� ������������ ���ĵǾ� �ִ� (0~data-count-1) ��ҵ鿡 ���������� ���� �����ϱ�
                    unsigned insertion_index = 0;
                    while (leaf_data_block->data_array[insertion_index].score < new_data.score) insertion_index++;
                    //insertion_index�� �������� ������ ���ο� �������� �������� ũ�ų� ����. �̰ͺ��� data_count - 1 �ε��������� �����͸� �� ĭ�� �̷��, insertion_index�� ��ġ�� new_data�� ��������.
                    for (int i = data_count - 1; insertion_index <= i; i--) {
                        leaf_data_block->data_array[i + 1] = leaf_data_block->data_array[i];
                    }
                    leaf_data_block->data_array[insertion_index] = new_data;

                    //���ο� ��带 ������ ���� �ƴϴ� ���ۺ��� ������Ʈ �� �ʿ�� ����.
                    //������Ʈ �� ���� ������ ���� ���ִ� ������ ������.
                    //TODO : ���� �����
                    return;
                }
                //�����ִٸ� ����� �����͸� ������ ��������. �̶�
                //�θ� ���� ��嵵 �� á�ٸ� ������ �����س�����. ���� �ð� �� �־��� ��쿣, ��Ʈ ��带 ���� �ϴ� �ͱ��� �Ž��� �ö󰣴�.
                else {
                    //�켱 ������ �����͵��� �����ؼ� ���� �������� ��������. �׷��� �ؾ� ���� ������ �� �� ���� ���̴�.
                    //�ϴ� ������ ��ġ�� ã��.
                    unsigned insertion_index = 0;
                    while (leaf_data_block->data_array[insertion_index].score < new_data.score) insertion_index++;

                    //insertion_index�� �������� ������ ���ο� �������� �������� ũ�ų� ����. �켱 ��(510 �ε���)�� ��Ҹ� ���� ������ �����ϰ�, insertion_index���� ����������(509 �ε�������)�� �����͸� �� ĭ�� �̷���.
                    //�׸��� insertion_index ��ġ�� �� �����͸� �����ؼ� ���������� �ϼ�����.
                    //TODO : ���� Ű�� ���� ū ��� ó��
                    BPlusTreeData last_data = leaf_data_block->data_array[510];
                    for (int i = 510 - 1; insertion_index <= i; i--) {
                        leaf_data_block->data_array[i + 1] = leaf_data_block->data_array[i];
                    }
                    leaf_data_block->data_array[insertion_index] = new_data;

                    //������忡 �̹� ���� �ִ� �����ʹ� 511��, ���⿡ ���ο� �����ͱ��� �ϳ� ���ϸ� 512���̴�.
                    //�׷��ٸ� 256���� �� ���� ���� ���� �����ϸ� �ȴ�. ���⼭ �θ�� �ø� ���� Ű�� ���� ���� ����� ���� ū Ű ���̴�.
                    BPlusTreeLeafDataBlock* second_leaf_data_block = new BPlusTreeLeafDataBlock{ {0, }, NULL };
                    for (int i = 0; i < 255; i++) {
                        second_leaf_data_block->data_array[i] = leaf_data_block->data_array[256 + i];
                        leaf_data_block->data_array[256 + i] = BPlusTreeData{ 0, 0 };
                    }
                    second_leaf_data_block->data_array[255] = last_data;

                    float division_key = leaf_data_block->data_array[255].score;
          
                    //������Ʈ �� leaf_data_block�� ���ҵǸ� ���� ������� seconde_data_block�� ����. �׸��� ���� ������� seconde_data_block�� ������ ������Ʈ ����. ���� ���� seconde_data_block�� ��� �ε����� number_of_nodes�� 1 ������Ų ������ ��������.
                    number_of_nodes++;
                    //TODO : ���� �����
                    delete leaf_data_block;
                    delete second_leaf_data_block;
                    leaf_data_block = NULL;
                    second_leaf_data_block = NULL;
                    
                    //������ ���� ���� �߾Ӱ� Ű�� �θ�� �÷��� �Ѵ�. �־��� ��� ��Ʈ������ �Ž��� �ö� �����ؾ� �Ѵ�.
                    unsigned division_node_index = search_route_stack.pop();

                    while (division_node_index != NULL) {
                        //�θ� ��尡 ������ �ű⿡ ���� �� �ִ��� Ȯ���غ���.
                        if (division_node_index != NULL) {
                            //�θ��� ��� �ε����� ���� �ش� ���� ��带 �ҷ�����.
                            BPlusTreeInnerDataBlock* division_node = new BPlusTreeInnerDataBlock{ {0,}, {0,} };
                            //TODO : ���� �����

                            //�ش� ��忡 ���� �� �ִ� Ű ���� ������ 1�� �ִ��� Ȯ������. inner node�� ���� Ű ������ -1.0���� �ʱ�ȭ�Ǿ� ������ 0 �̻������� Ȯ���ϸ� �ȴ�.
                            unsigned inner_key_count = 0;
                            while (division_node->score_array[inner_key_count] >= 0.0) inner_key_count++;

                            //���� ������ ������ division_key�� ���� ������ �°� �����ϰ�, ���ο� second_leaf_data_block�� ��� �ε����� �ڽ� �����ͷμ� ������ �°� �����ϰ� ������ �ȴ�.
                            if (inner_key_count < 511) {
                                //TODO : ��̿� �����͸� ���������� �°� ����(Ű ���, �ڽ� ��� �ε��� ���)
                                return;
                            }
                            //���� ������ ���ٸ�, �� ���� ��嵵 �����ϰ�, �θ� ���� Ű ������ �Ž��� �ö󰣴�.
                            else {
                                //�켱 ������ ���� Ű�� �����ؼ� ���� �������� ��������. �׷��� �ؾ� ���� ������ �� �� ���� ���̴�.
                                //�ϴ� ������ ��ġ�� ã��.
                                unsigned inner_insertion_index = 0;
                                while (division_node->score_array[inner_insertion_index] < division_key) inner_insertion_index++;

                                //inner_insertion_index�� ����Ű�� ���� Ű�� �ڽ��� ���ҷ� �ö�� division_key�� �������� ũ�ų� ����. �켱 ��(510 �ε���)�� Ű�� ���� ������ �����ϰ�, inner_insertion_index���� ����������(509 �ε�������)�� �����͸� �� ĭ�� �̷���.
                                //�׸��� inner_insertion_index ��ġ�� �� �����͸� �����ؼ� ���������� �ϼ�����.
                                //�ڽ� ��� �ε��� ��̵���, ���ҷ� ������ ���ο� ����� �ε����� �����ؾ� �ϹǷ�, inner_insertion_index+1�� �ε����� ��ġ�� �ڽ� ��� �ε���(���� ���ԵǴ� Ű�� ������ �ڽ�)���� �� �ϳ��� �а�, �ش� �ڸ����ٰ� ���� ���� ��带 ����������.
                                //TODO : ���� Ű�� ���� ū ��� ó��, �Ʒ� �������� �ڽ� ��� �ε��� ������ �´��� �� �� �� Ȯ���ϱ�.
                                float last_key = division_node->score_array[510];
                                unsigned last_child_node_index = division_node->child_node_index_array[511];

                                for (int i = 510 - 1; inner_insertion_index <= i; i--) {
                                    division_node->score_array[i + 1] = division_node->score_array[i];
                                    division_node->child_node_index_array[i + 2] = division_node->score_array[i + 1];
                                }
                                division_node->child_node_index_array[inner_insertion_index + 2] = division_node->child_node_index_array[inner_insertion_index + 1];

                                division_node->score_array[inner_insertion_index] = division_key;
                                division_node->child_node_index_array[inner_insertion_index+1] = number_of_nodes;


                                //������������ ���ĵ� Ű�� �ڽ� ��� �ε��� ��̵��� �ݹ����� ������ ���ο� ���� ���� �����ϱ�
                                //TODO : �� ���� ��� �����ؼ� Ű�� �ڽ� ��� �ε����� �� �����ϱ�
     

                                //������Ʈ �� ���γ���, ���� ������ ���� ��带 ���Ͽ� ����. ���� ������ ���� ����� �ε����� number_of_nodes�� 1 ������Ų ������ �Ѵ�.
                                //TODO : ���� �����
                            }
                        }
                    }
                    //������� �Դٴ� ���� ��Ʈ��忡�� ������ �Ͼ���� �ǹ��Ѵ�.
                    
                }
            }
            //���� ��尡 �ƴ϶�� Ž���� ���� ������ �ڽ� ��� �ε����� �˾Ƴ��� search_index�� ��������.
            //���� ����� Ű ���ھ �����Ǵ� ���� �����ʹ� ���������� ������忡 ��ġ�Ѵ�. ���� �ش� Ű�� �ߺ��Ǵ� Ű�� �����͵鵵 �װ��� ��ġ�Ѵ�.
            //���� ã���� �ϴ� �������� Ű�� ���� ����� Ű�� ��ġ�ϸ� ���� �ڽ����� �������� �Ѵ�.
            //�̶� �̹��� �־��� ����� �ε����� Ž�� ��θ� �����ϴ� ���ÿ��ٰ� Ǫ�� �ص���. ������ ��Ʈ�� ���� �ö� �� �ʿ��ϴ�.
            else {
                //TODO
                return;
            }
        }

    }

    //������ �䱸 ���� ��, Ž�� ���� ������ �����ϴ� �л� �� ���� ���� ���Ե� �л��� �˾Ƴ��� ���ؼ���,
    //���Ե� ������� student.dat ���Ͽ� ���� �ִ� �л� �� ���� �տ� ���� �л��� �˾Ƴ��� �ȴ�.
    //�̸� �˾Ƴ��� ���ؼ� B+Ʈ���� ��ġ������ ������ �����ϴ� �л��� ������ student.dat ���ϳ� �� ��ȣ ��, ���� �ּ� ���� �˾Ƴ��� ��ȯ����.
    //�̸� ȣ���� �Լ������� �ش� ������ ������ student.dat ������ �ش� ���� ���� Ȯ���Ͽ�, 
    //�� ������ ���� ������ �����ϴ� �л� �� ���� ���� ���� �л��� ��ȯ�ϸ� �� ���̴�.
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

//������ �䱸������ ������ �����ϴ� �л��� ����, �� �߿� ���� ���� ���Ե� �л��� �˾Ƴ��� ���̴�.
//���� ��ȯ�ڸ� ���ؼ� �����ϰ�, ���� ���� ���Ե� �л��� ��� �Ű������� student�� ������ �ȴ�.
unsigned searchStudent(float minimum_score, float max_score, Student* student) {
    student->ID=12345; student->name="abcd"; student->score=80.5; student->advisorID=654321;
    return 10;;
}
