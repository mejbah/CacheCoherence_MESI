#ifndef _COCHE_CORE_H_

#include "cache.hh"
#include "cache_util.hh"
#include <pthread.h>
class CacheCore{

private:
  CacheL1 *_cache;
  int _cores;
  STATE *cache_event; //store cache event just before access, thats what pbi does??
  pthread_mutex_t cache_lock;
  CacheCore(){};

public:
  static CacheCore& getInstance(){
    static char buf[sizeof(CacheCore)];
    static CacheCore *singleObject = new (buf) CacheCore();
    return *singleObject;
  }

  void initialize(int cores ){
    _cache = new CacheL1[cores];
    _cores = cores;
    cache_event = new STATE[_cores];
    pthread_mutex_init(&cache_lock, NULL);
  }

  void read( ADDRINT address, int core ){
    assert(address!=0);
    pthread_mutex_lock(&cache_lock);
    //access(address, core);
    unsigned int setIndex = CacheUtil::getInstance().getSetIndex(address);
    unsigned int tag = CacheUtil::getInstance().getTag(address);
    //assert(tag != 0); 
    //if(tag == 0) return; //TODO: fix this??
    //fprintf(stderr, "read core %d setIndex %u, tag %u\n", core, setIndex, tag);

    //local read
    bool hit;
    CacheSet *set = _cache[core].getSet(setIndex);
    CacheLine *line = set->access(tag,&hit);

    //update perf event
    cache_event[core] = line->getState(); 


    //fprintf(stderr, "Line state before %d\n", line->getState());
    //bus read
    if( hit && (line->getState() == EXCLUSIVE || line->getState() == MODIFIED)){
      pthread_mutex_unlock(&cache_lock);
      return;
    }
    int read_hit = false;
    for(int i= 0; i< _cores; i++ ){
      if( i != core ){
        CacheSet *remote_set = _cache[i].getSet(setIndex);
        CacheLine *remote_line = remote_set->remoteAccess(tag);

        if( remote_line != NULL ) {
          //fprintf(stderr, "Remote line in %d state before %d\n", i, remote_line->getState()); 
          if(remote_line->getState() != INVALID) {
            read_hit = true;
            if(remote_line->getState() != SHARED){
              remote_line->setState(SHARED);
            }
          }
        }

      }
    }
    if(!read_hit){
      //assert( line->getState() != SHARED );//TODO:check why this assertion fails:: cache eviction???
      if(line->getState() == INVALID)
         line->setState(EXCLUSIVE);
      //if line is modified then remain so
    }
    else {
      line->setState(SHARED);
    }
    pthread_mutex_unlock(&cache_lock);
    
  }
  void write( ADDRINT address, int core){
    //access(address, core);
    assert(address != 0);
    pthread_mutex_lock(&cache_lock);
    unsigned int setIndex = CacheUtil::getInstance().getSetIndex(address);
    unsigned int tag = CacheUtil::getInstance().getTag(address);
    //assert(tag != 0);
    //if(tag == 0 ) return; //TODO: fix this??
    //fprintf(stderr, "write core %d  setIndex %d tag %d\n",core, setIndex,tag);

    //local write
    bool hit;
    CacheSet *set = _cache[core].getSet(setIndex);
    CacheLine *line = set->access(tag, &hit);

    //update perf event
    cache_event[core] = line->getState();

    //fprintf(stderr, "Line state before %d\n", line->getState());
    if(hit && line->getState() == MODIFIED ) {
      pthread_mutex_unlock(&cache_lock);
      return;
    }
     
    line->setState(MODIFIED);

    //bus write invalidate
    for(int i=0; i<_cores; i++){
      if(i != core){
        CacheSet *remote_set = _cache[i].getSet(setIndex);
        CacheLine *remote_line = remote_set->remoteAccess(tag);
        
        if( remote_line != NULL ){ //remote hit
          //fprintf(stderr, "Remote line in %d state before %d\n",i,  remote_line->getState());
          remote_line->setState(INVALID); 
        }
      }
    }
    pthread_mutex_unlock(&cache_lock);
  }


  void access( unsigned int address, int core){
    unsigned int setIndex = CacheUtil::getInstance().getSetIndex(address);
    unsigned int tag = CacheUtil::getInstance().getTag(address);
    bool hit;
    //fprintf(stderr, "setIndex %d\n", setIndex);
    CacheSet *set = _cache[core].getSet(setIndex);
    CacheLine *line = set->access(tag, &hit);


    //fprintf(stderr, "tag %u foundtag %u\n", tag, line->getTag());
    
  }

  STATE getCacheEvent(int core){
    return cache_event[core];
  }


};


#endif
