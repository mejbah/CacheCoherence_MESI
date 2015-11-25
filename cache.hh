#ifndef _CACHE_LINE_
#define _CACHE_LINE_


#define ADDR_SIZE 64
#define CACHE_SIZE_KB 32
#define BLOCK_SIZE 32
#define ASSOC 4


enum STATE { MODIFIED, EXCLUSIVE, SHARED,  INVALID};

// 19bit tag | 8bit set | 5bit block


class CacheLine {

private:
  STATE state; 
  unsigned int tag;
  bool valid;


public:
  CacheLine(){
    tag = 0;
    state = INVALID;
    valid = false;
  }
  ~CacheLine(){};

  unsigned int getTag(){return tag;}
  void setTag(unsigned int _tag ) { tag = _tag; }

  bool getValid(){ return valid; }
  void setValid(bool _valid){ valid = _valid; }

  STATE getState(){ return state; }
  void setState( STATE _state ) { state =_state; }

};


class CacheSet {

private:
  CacheLine _cache_line[ASSOC];
  int MRU; // most recent used bit for replacement
  

public:
  CacheSet(){ 
  };
  ~CacheSet(){};
  
  int getMRU() { return MRU; }
  
  CacheLine* getLine(int assoc){return &_cache_line[assoc];}

  CacheLine* access( unsigned int tag, bool *hit ){
    *hit = false;
    for(int i=0; i<ASSOC; i++){
      //TODO: check valid
      //if(_cache_line[i].getState != INVALID){
        if(_cache_line[i].getTag() == tag && _cache_line[i].getValid()){
          //fprintf(stderr, "Hit %d\n", i);
          *hit = true;
          MRU = i;
          break;
        }
      //}
    }
    if( *hit == false ){ //TODO:LRU replacement

      if(MRU != (ASSOC-1)){
        MRU  = MRU + 1;
      }
      else {
        MRU = 0;
      }
      //fprintf(stderr, "Miss %d\n", MRU);
      _cache_line[MRU].setTag(tag);
      _cache_line[MRU].setValid(true);

    }
    return &_cache_line[MRU];

  }

  CacheLine* remoteAccess( unsigned int tag ){
    bool hit = false;  
    for(int i=0; i<ASSOC; i++){
      //TODO: check valid
      //if(_cache_line[i].getState != INVALID){
        if(_cache_line[i].getTag() == tag && _cache_line[i].getValid()){
          //fprintf(stderr, "Remote Hit %d\n", i);
          hit = true;
          MRU = i;
          return &_cache_line[i];
        }
      //}
    }
    return NULL;
  }

#if 0
  CacheLine* getLRU() { return _cache_line.back(); }

  void check( unsigned int tag ){
    std::list<CacheLine>::iterator it;
    bool hit = false;
    for(it = _cache_line.begin(); it != _cache_line.end(); it++){
      if(it->getTag()== tag){
        hit = true;
        if(it != _cache_line.begin()){
          _cache_line.erase(it);
          _cache_line.push_front(it);
        }
      }
    }
  }
#endif

};


class CacheL1 {

private:
  CacheSet *sets;
public:
  
  CacheL1(){
    int no_of_blocks = (CACHE_SIZE_KB * 1024) / BLOCK_SIZE;
    int no_of_sets = no_of_blocks / ASSOC;
    sets = new CacheSet[no_of_sets];   
  }

  CacheSet* getSet( int set_index ){
    return &sets[set_index];
  }
  

};
#endif
