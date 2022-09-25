#include <key_down_check.h>
#include <X11/Xutil.h>
key_down_check_t* key_down_check_new(void){
    key_down_check_t* this=(malloc(sizeof(key_down_check_t)));
    EXIT_IF_NULL(this,key_down_check_t*);
    this->keys=malloc(sizeof(const char*)*1);//No need to malloc if 0. Just realloc.
    EXIT_IF_NULL(this->keys,const char*);
    this->len=0;
    return this;
}
bool key_down_check_add(key_down_check_t* this,const char* add_key){
    for(int i=0;i<this->len;i++){
        if(!strcmp(this->keys[i],add_key)) return false;//Don't add duplicate keys.
    }
    this->keys[this->len++]=add_key;
    this->keys=(const char**)(realloc(this->keys,sizeof(const char*)*(this->len+1)));
    EXIT_IF_NULL(this->keys,const char*);
    return true;
}
bool key_down_check_remove(key_down_check_t* this,const char* rem_key){
    for(int i=0;i<this->len;i++){
        if(!strcmp(this->keys[i],rem_key)){//If found, just move the last key in the array where the deleted_key is, and NULL last.
            this->keys[i]=this->keys[this->len-1];
            this->keys[(this->len--)-1]=NULL;
            this->keys=(const char**)(realloc(this->keys,sizeof(const char*)*(this->len+1)));
            EXIT_IF_NULL(this->keys,const char*);
            return true;
        }
    }
    return false;
}
void key_down_check_print(const key_down_check_t* this){
    printf("Keys pressed: ");
    for(int i=0;i<this->len;i++){
        printf("%s ",this->keys[i]);
    }
    puts("");
}
void key_down_check_key_up(key_down_check_t* this,xdo_t* xdo_obj,Window window){
    for(int i=0;i<this->len;i++){
        xdo_send_keysequence_window_up(xdo_obj,window,this->keys[i],0);
    }
}
void key_down_check_free(key_down_check_t* this){
    free(this->keys);
    free(this);
}
bool _KeybindDisable=false;
//Removes keybinds except for escape key.
void _GlobalKeybindDisable(Display* xdpy,int scr,bool d,const callback_t* cb_list,size_t cb_list_len){
    _KeybindDisable=d;
    puts(_KeybindDisable?"Escape key pressed. Keybinds are currently disabled. To reenable, press Escape key again.":"Escape key pressed. Keybinds are currently enabled.");
    for(size_t i=0;i<cb_list_len;i++) d?XUngrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr)):XGrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
}
void wait_for_keypress(Display* xdpy,KeySym ks){
    const callback_t to_disable={.ks=ks};
    int scr=DefaultScreen(xdpy);
    XEvent e={0};
    bool finished=false;
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,ks),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    while(!finished){
        usleep(100000);
        while(XPending(xdpy)){
            XNextEvent(xdpy,&e);
            if(!_KeybindDisable){
                if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,ks)) finished=true;
                else if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)) _GlobalKeybindDisable(xdpy,scr,true,&to_disable,1);
            }else if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)) _GlobalKeybindDisable(xdpy,scr,false,&to_disable,1);
        }
    }
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,ks),None,RootWindow(xdpy,scr));
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr));
}
void keypress_loop(Display* xdpy,const callback_t* cb_list,size_t cb_list_len){
    int scr=DefaultScreen(xdpy);
    XEvent e={0};
    bool keep_looping=true;
    for(size_t i=0;i<cb_list_len;i++) XGrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    XGrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr),False,GrabModeAsync,GrabModeAsync);
    while(keep_looping){
        usleep(100000);
        while(XPending(xdpy)){
            XNextEvent(xdpy,&e);
            if(e.type==KeyPress){
                if(!_KeybindDisable){
                    if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)){
                        _GlobalKeybindDisable(xdpy,scr,true,cb_list,cb_list_len);
                        break;
                    } 
                    for(size_t i=0;i<cb_list_len;i++){
                        if(e.xkey.keycode==XKeysymToKeycode(xdpy,cb_list[i].ks)){
                            keep_looping&=cb_list[i].func(cb_list[i].arg);//Keep looping until one is false.
                            break;
                        }
                    }
                }else if(e.type==KeyPress&&e.xkey.keycode==XKeysymToKeycode(xdpy,XK_Escape)) _GlobalKeybindDisable(xdpy,scr,false,cb_list,cb_list_len);
            }
        }
    }
    for(size_t i=0;i<cb_list_len;i++) XUngrabKey(xdpy,XKeysymToKeycode(xdpy,cb_list[i].ks),None,RootWindow(xdpy,scr));
    XUngrabKey(xdpy,XKeysymToKeycode(xdpy,XK_Escape),None,RootWindow(xdpy,scr));
}
bool CallbackEndLoop(void* v){
    (void)v;
    return false;
}
bool _boolean_edit_func(void* b_v){
    *(((_boolean_edit_t*)b_v)->p)=((_boolean_edit_t*)b_v)->v;
    return false;
}
