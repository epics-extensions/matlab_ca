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

/** Hash, mapping integer keys to 'T' pointers. */ 
template<class T>
class IntHash
{
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

    void remove (int key)
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
                delete node;
                Elements--;
                return;
            }
            prev = node;
            node = node->getNext();
        }
    }

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

#if 0
class IntHashIterator
{
private:
	IntHash     * hashTbl;
	IntHashNode * node;
	int           idx;	

public:
	IntHashIterator( IntHash * HashTbl );	
	~IntHashIterator( void );
	
	inline void * first       ( void );
	inline void * operator ++ ( void );
	inline void * operator ++ ( int x );
	inline int    key         ( void );
	inline void * data        ( void );
};

inline IntHashIterator::IntHashIterator(IntHash * HashTbl)
	: hashTbl(HashTbl), idx(0), node(NULL)
	{
	}
	
inline IntHashIterator::~IntHashIterator( void ) 
	{
	}
	
inline void * IntHashIterator::first ( void )
	{
	if(hashTbl!=NULL)
		{
		for(idx = 0; idx<IntHash::HASH_CNT &&
		   (node = hashTbl->nodes[idx])==NULL; idx++);
		}
	else node = NULL;
	return (node!=NULL)?node->hashData:NULL;
	}
	
inline void * IntHashIterator::operator ++ ( void )
	{
	if(hashTbl!=NULL)
		{
		if(node==NULL) first();
		else if(node->next!=NULL) node = node->next;
		else 
			{
			node = NULL;
			do 	{
				idx++;
				} while(idx<IntHash::HASH_CNT && 
				       (node = hashTbl->nodes[idx])==NULL);
			}
		}
	else node = NULL;
	return (node!=NULL)?node->hashData:NULL;
	}
	
inline void * IntHashIterator::operator ++ ( int x )
	{
	if(hashTbl!=NULL && x==0)
		{
		if(node==NULL) first();
		else if(node->next!=NULL) node = node->next;
		else 
			{
			node = NULL;
			do 	{
				idx++;
				} while(idx<IntHash::HASH_CNT && 
				       (node = hashTbl->nodes[idx])==NULL);
			}
		}
	else node = NULL;
	return (node!=NULL)?node->hashData:NULL;
	}

inline int IntHashIterator::key ( void )
	{
	return (node!=NULL)?node->hashInt:NULL;
	}
	
inline void * IntHashIterator::data ( void )
	{
	return (node!=NULL)?node->hashData:NULL;
	}

#endif

#endif /* _INT_HASH_H_ */
