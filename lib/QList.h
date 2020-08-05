#ifndef GO_LIST_H
#define GO_LIST_H
template <typename T>
struct Lnode
{
    Lnode(T v):v(v),next(nullptr){}
    T      v;
    Lnode* next;
};


template <typename T>
struct QList
{
    Lnode<T>* head;
    Lnode<T>* tail;
    void put(T v)
    {
        Lnode<T> *node = new Lnode<T>(v);
        if(head == nullptr){
            head = node;
            tail = node;
        }else{
            tail->next = node;
            tail = node;
        }
    }
    bool isEmpty(){return head == nullptr;}
    T pop()
    {
        Lnode<T> *node = head;
        if(head == tail){
            T r = node->v;
            delete node;
            head = nullptr;
            tail = nullptr;
            return r;
        }else{

            head = node->next;
            T r = node->v;
            delete node;
            return r;
        }
    }
};

#endif //GO_LIST_H
