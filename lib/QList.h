#ifndef GO_LIST_H
#define GO_LIST_H
template <typename T>
struct Lnode{
    Lnode(T v):v(v),next(nullptr){}
    T      v;
    Lnode* next;
};


template <typename T>
struct QList{
    void put(T v)
    {
        Lnode<T> *node = new Lnode<T>(v);
        if(head == nullptr){
            head = node;
        }else{
            node->next = head;
            head = node;
        }
    }
    bool isEmpty(){return head == nullptr;}
    T pop()
    {
        Lnode<T> *node = head;
        head = node->next;
        T r = node->v;
        delete node;
        return r;
    }
    Lnode<T>* head;
};

#endif //GO_LIST_H
