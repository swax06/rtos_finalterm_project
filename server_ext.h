extern int myRead(int sd, char *buff);
extern int findInd(int sid);
extern void del_entry(int sid);
extern void send_names(int sid);
extern void send_groups(int sid);
extern void* s_call(void *inp);
extern void* v_call(void *inp);