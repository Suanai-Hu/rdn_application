#ifndef __RDN_API_H__
#define __RDN_API_H__

#define KEY 7788

int rdn_query(char* path);
void rdn_show(char* path);
int rdn_commit(char* path);
int rdn_get(char* path, char* attr, char* value, int lenth);
int rdn_set(char* path, char* attr, char* value);

typedef struct _attrNode {
    
    char name[32];  	/*the name of the property */
	char value[64];		/*the value of the property */
    int next;  			/*next sibling link */
    int prev;  			/*previous sibling link */
} attrNode;

typedef struct _cfgNode {
    char name[32];      /* the name of the node, or the entity */
    int children; 		/* parent->childs link */
    int next;   		/* next sibling link */
    int prev;   		/* previous sibling link */
    int attr;			/* properties list */
} cfgNode;

#endif


