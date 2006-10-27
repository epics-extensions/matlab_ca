#ifndef _INT_HASH_H_
#define _INT_HASH_H_ 1

#include <string.h>

/** One node in a hash, mapping integer keys to 'T' pointers. */ 
template<class T>
class IntHashNode
{
public:
    IntHashNode(int key, T *value, IntHashNode *next)
      : key(key), value(value), next(next)
    {}
    
    int getKey() const
    {   return key; }

    T *getValue() const
    {   return value; }
    
    void setValue(T *value)
    {   this->value = value; }
    
    IntHashNode *getNext() const
    {   return next; }

    void setNext(IntHashNode *next)
    {   this->next = next; }
    
private:
	int            key;
	T              *value;
	IntHashNode<T> *next;
};

// forward
template<class T>
class IntHashIterator;

/** Hash, mapping integer keys to 'T' pointers. */ 
template<class T>
class IntHash
{
    friend class IntHashIterator<T>;
    
public:
    IntHash  ( void )
        : Elements(0)
    {
        memset(nodes, 0, sizeof(nodes));
    }

    ~IntHash ( void )
    {
        for (int i=0; i<HASH_CNT; i++)
        {
            IntHashNode<T> *node = nodes[i];
            while (node)
            {
                IntHashNode<T> *tmp = node->getNext();
                delete node;
                node = tmp;
            }
        }
    }

    void insert(int key, T *value)
    {
        unsigned char idx = hash(key);
        IntHashNode<T> *node = nodes[idx];
        
        while (node)
        {
            if (node->getKey() == key)
            {   // Found, update value
                node->setValue(value);
                return;
            }
            node = node->getNext();
        }
        // Not found, add to front
        nodes[idx] = new IntHashNode<T>(key, value, nodes[idx]);
        Elements++;
    }

    /** Remove element with given key.
     *  <p>
     *  <b>NOTE:</b>
     *  Calling 'remove' invalidates any iterator
     *  that is currently attached to the hash!
     * 
     *  @return The value for that key or 0.
     */
    T *remove (int key)
    {
        unsigned char idx = hash(key);
        IntHashNode<T> *prev = 0, *node = nodes[idx];
        
        while (node)
        {
            if (node->getKey() == key)
            {   // Unlink 'node' and delete
                if (prev)
                    prev->setNext(node->getNext());
                else           
                    nodes[idx] = node->getNext();
                T *value = node->getValue();
                delete node;
                Elements--;
                return value;
            }
            prev = node;
            node = node->getNext();
        }
        return 0;
    }

    /** Find an entry.
     *  @return The value for that key or 0.
     */
    T *find (int key)
    {
        unsigned char idx = hash(key);
        IntHashNode<T> *node = nodes[idx];
        while (node)
        {
            if (node->getKey() == key)
                return node->getValue();
            node = node->getNext();
        }
        return 0;
    }

    int size ()
    {   return Elements; }
    
private:
    enum { HASH_CNT=256 };

    int hash (int key)
    {
        return (key % HASH_CNT);
    }

    IntHashNode<T> *nodes[HASH_CNT];

    int Elements;
};

/** Iterator for the hash. */ 
template<class T>
class IntHashIterator
{
public:
    IntHashIterator(IntHash<T> &hash)
        : hash(hash), idx(-1), node(0)
    {}
    
    T *next()
    {   // init: node 0, idx -1
        if (node)
            node = node->getNext();
        if (!node)
        {
            while (++idx < hash.HASH_CNT)
            {
                node = hash.nodes[idx];
                if (node)
                    break;
            }
        }
        return getValue();
    }
    
    int getKey() const
    {
        return node ? node->getKey() : 0;
    }
    
    T *getValue() const 
    {
        return node ? node->getValue() : 0;
    }

private:
    IntHash<T>    &hash;
    int           idx;  
    IntHashNode<T> *node;
};


#endif /* _INT_HASH_H_ */
