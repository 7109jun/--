// JUP v1 -- single-file reference implementation (C++17, standard library only)
//
// Binary format (all multibyte fields are little endian):
// Header (64 bytes): magic "JUP1", u16 version=1, u8 endian=1, u8 header_size=64,
// u32 flags, u32 entry, u32 instruction_count, u32 section_count, u64 directory_offset,
// u64 file_size, u64 integrity_offset, u64 original_size, u32 header_crc32, u32 reserved.
// It is followed by five 32-byte directory entries: u32 type, u32 flags, u64 offset,
// u64 size, u32 crc32, u32 reserved. Sections: PROG, CONS, DATA, AI__, META.
// Program instructions are fixed 16-byte records: u8 opcode,u8 type,u16 flags,i32 a,b,c.
// Lengths and offsets are fixed-width u64; this avoids ambiguous integer encodings.
// Integrity is SHA-256 of all bytes before the integrity section. META begins with the
// original SHA-256. The format has no file, network, process, or shell instructions.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace jup {

static const uint16_t VERSION = 1;
static const uint32_t FLAG_AI = 1;
static const uint32_t S_PROG=0x474f5250, S_CONS=0x534e4f43, S_DATA=0x41544144,
                      S_AI=0x5f5f4941, S_META=0x4154454d, S_INTEGRITY=0x4745544e;

enum class Type : uint8_t { NONE, U8,U16,U32,U64,I8,I16,I32,I64,F32,F64,BOOL,BYTES,STRING };
enum class Op : uint8_t {
  NOP, CONST, PUSH, POP, LOAD, STORE, ADD, SUB, MUL, DIV, MOD, AND, OR, XOR, NOT, SHL, SHR,
  CMP, EQ, NE, LT, GT, LE, GE, JUMP, JZ, JNZ, CALL, RET, COPY, REPEAT, RANGE, WRITE, AI_RUN, HALT
};
enum class AIOp : uint8_t { CREATE, LOAD, ADD, SUB, MUL, DOT, MATMUL, TRANSPOSE, RELU, SIGMOID, TANH, SOFTMAX, RESULT };

struct Error : std::runtime_error { using std::runtime_error::runtime_error; };

template<class T> static void put(std::vector<uint8_t>& v, T x) {
  static_assert(std::is_integral<T>::value, "integer only");
  using U=typename std::make_unsigned<T>::type; U u=(U)x;
  for(size_t i=0;i<sizeof(T);++i) v.push_back((uint8_t)(u>>(i*8)));
}
template<class T> static T get(const std::vector<uint8_t>& v,size_t& p) {
  static_assert(std::is_integral<T>::value, "integer only");
  if(p+sizeof(T)>v.size()) throw Error("truncated binary data");
  using U=typename std::make_unsigned<T>::type; U u=0;
  for(size_t i=0;i<sizeof(T);++i) u|=(U)v[p++]<<(i*8);
  return (T)u;
}
static void put64at(std::vector<uint8_t>& v,size_t p,uint64_t x){ for(int i=0;i<8;i++) v[p+i]=(uint8_t)(x>>(i*8)); }
static void put32at(std::vector<uint8_t>& v,size_t p,uint32_t x){ for(int i=0;i<4;i++) v[p+i]=(uint8_t)(x>>(i*8)); }
static uint32_t crc32(const uint8_t* data,size_t n) { uint32_t c=~0u; for(size_t i=0;i<n;i++){c^=data[i];for(int k=0;k<8;k++)c=(c>>1)^((c&1)?0xedb88320u:0);}return ~c; }
static uint32_t crc32(const std::vector<uint8_t>& v){return crc32(v.data(),v.size());}

// Small self-contained SHA-256 implementation used for original identity and file integrity.
class SHA256 {
  uint32_t h[8]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
  uint8_t block[64]={}; size_t used=0; uint64_t bits=0;
  static uint32_t r(uint32_t x,int n){return (x>>n)|(x<<(32-n));}
  void transform(const uint8_t* b){ static const uint32_t k[64]={
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x9cca4f24,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    uint32_t w[64]; for(int i=0;i<16;i++)w[i]=((uint32_t)b[4*i]<<24)|((uint32_t)b[4*i+1]<<16)|((uint32_t)b[4*i+2]<<8)|b[4*i+3];
    for(int i=16;i<64;i++)w[i]=(r(w[i-15],7)^r(w[i-15],18)^(w[i-15]>>3))+w[i-16]+(r(w[i-2],17)^r(w[i-2],19)^(w[i-2]>>10))+w[i-7];
    uint32_t a=h[0],b0=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],x=h[7];
    for(int i=0;i<64;i++){uint32_t t1=x+(r(e,6)^r(e,11)^r(e,25))+((e&f)^((~e)&g))+k[i]+w[i];uint32_t t2=(r(a,2)^r(a,13)^r(a,22))+((a&b0)^(a&c)^(b0&c));x=g;g=f;f=e;e=d+t1;d=c;c=b0;b0=a;a=t1+t2;}
    h[0]+=a;h[1]+=b0;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=x;
  }
public:
  void update(const uint8_t* p,size_t n){bits+=(uint64_t)n*8;while(n){size_t take=std::min(n,64-used);std::memcpy(block+used,p,take);used+=take;p+=take;n-=take;if(used==64){transform(block);used=0;}}}
  std::array<uint8_t,32> finish(){uint64_t total=bits;uint8_t one=0x80;update(&one,1);uint8_t zero=0;while(used!=56)update(&zero,1);for(int i=7;i>=0;i++){uint8_t q=(uint8_t)(total>>(i*8));update(&q,1);}std::array<uint8_t,32> o{};for(int i=0;i<8;i++)for(int j=0;j<4;j++)o[4*i+j]=(uint8_t)(h[i]>>(24-8*j));return o;}
};
static std::array<uint8_t,32> sha256(const std::vector<uint8_t>& x){SHA256 s;s.update(x.data(),x.size());return s.finish();}
static std::string hex(const std::array<uint8_t,32>& a){std::ostringstream o;for(auto b:a)o<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)b;return o.str();}

struct Instruction { Op op=Op::NOP; Type type=Type::NONE; uint16_t flags=0; int32_t a=0,b=0,c=0; };
static std::vector<uint8_t> encodeProgram(const std::vector<Instruction>& p){std::vector<uint8_t> o;o.reserve(p.size()*16);for(auto i:p){put<uint8_t>(o,(uint8_t)i.op);put<uint8_t>(o,(uint8_t)i.type);put<uint16_t>(o,i.flags);put<int32_t>(o,i.a);put<int32_t>(o,i.b);put<int32_t>(o,i.c);}return o;}
static std::vector<Instruction> decodeProgram(const std::vector<uint8_t>& x){if(x.size()%16)throw Error("program section is not instruction aligned");std::vector<Instruction> p;size_t q=0;while(q<x.size()){Instruction i;i.op=(Op)get<uint8_t>(x,q);i.type=(Type)get<uint8_t>(x,q);i.flags=get<uint16_t>(x,q);i.a=get<int32_t>(x,q);i.b=get<int32_t>(x,q);i.c=get<int32_t>(x,q);p.push_back(i);}return p;}

struct Tensor { uint32_t rows=0, cols=0; std::vector<float> x; size_t size()const{return x.size();} };
struct AIInstruction { AIOp op=AIOp::CREATE; int32_t a=0,b=0,c=0; };
class JupAIRuntime {
public:
  static constexpr size_t MAX_TENSOR_BYTES=64ull*1024*1024, MAX_OPS=1000000;
  std::array<Tensor,16> t; std::vector<float> constants; const Tensor* resultPtr=nullptr; uint64_t operations=0, memory=0;
  void create(int id,int r,int c){if(id<0||id>=16||r<=0||c<=0)throw Error("invalid tensor shape or id");if(!t[id].x.empty())throw Error("AI tensor slot may only be initialized once");uint64_t n=(uint64_t)r*c;if(n>MAX_TENSOR_BYTES/4||memory+n*4>MAX_TENSOR_BYTES)throw Error("AI tensor memory limit exceeded");t[id]={(uint32_t)r,(uint32_t)c,std::vector<float>((size_t)n)};memory+=n*4;}
  Tensor& at(int id){if(id<0||id>=16||t[id].x.empty())throw Error("AI tensor is not initialized");return t[id];}
  void run(const std::vector<AIInstruction>& p){for(const auto& i:p){if(++operations>MAX_OPS)throw Error("AI operation limit exceeded"); switch(i.op){
    case AIOp::CREATE:create(i.a,i.b,i.c);break;
    case AIOp::LOAD:{Tensor& z=at(i.a);if(i.b<0||i.c<0||(size_t)i.b+(size_t)i.c>constants.size()||(size_t)i.c>z.size())throw Error("AI constant range invalid");std::copy(constants.begin()+i.b,constants.begin()+i.b+i.c,z.x.begin());break;}
    case AIOp::ADD: case AIOp::SUB: case AIOp::MUL:{Tensor&a=at(i.a),&b=at(i.b);if(a.rows!=b.rows||a.cols!=b.cols)throw Error("AI elementwise shape mismatch");create(i.c,a.rows,a.cols);for(size_t k=0;k<a.size();k++)t[i.c].x[k]=i.op==AIOp::ADD?a.x[k]+b.x[k]:i.op==AIOp::SUB?a.x[k]-b.x[k]:a.x[k]*b.x[k];break;}
    case AIOp::DOT:{Tensor&a=at(i.a),&b=at(i.b);if(a.size()!=b.size())throw Error("AI dot shape mismatch");create(i.c,1,1);float s=0;for(size_t k=0;k<a.size();k++)s+=a.x[k]*b.x[k];t[i.c].x[0]=s;break;}
    case AIOp::MATMUL:{Tensor&a=at(i.a),&b=at(i.b);if(a.cols!=b.rows)throw Error("AI matmul shape mismatch");create(i.c,a.rows,b.cols);for(uint32_t r=0;r<a.rows;r++)for(uint32_t c=0;c<b.cols;c++){float s=0;for(uint32_t k=0;k<a.cols;k++)s+=a.x[(size_t)r*a.cols+k]*b.x[(size_t)k*b.cols+c];t[i.c].x[(size_t)r*b.cols+c]=s;}break;}
    case AIOp::TRANSPOSE:{Tensor&a=at(i.a);create(i.b,a.cols,a.rows);for(uint32_t r=0;r<a.rows;r++)for(uint32_t c=0;c<a.cols;c++)t[i.b].x[(size_t)c*a.rows+r]=a.x[(size_t)r*a.cols+c];break;}
    case AIOp::RELU: case AIOp::SIGMOID: case AIOp::TANH:{Tensor&a=at(i.a);create(i.b,a.rows,a.cols);for(size_t k=0;k<a.size();k++){float v=a.x[k];t[i.b].x[k]=i.op==AIOp::RELU?std::max(0.0f,v):i.op==AIOp::SIGMOID?1.0f/(1.0f+std::exp(-v)):std::tanh(v);}break;}
    case AIOp::SOFTMAX:{Tensor&a=at(i.a);create(i.b,a.rows,a.cols);for(uint32_t r=0;r<a.rows;r++){float mx=-std::numeric_limits<float>::infinity(),s=0;for(uint32_t c=0;c<a.cols;c++)mx=std::max(mx,a.x[(size_t)r*a.cols+c]);for(uint32_t c=0;c<a.cols;c++)s+=(t[i.b].x[(size_t)r*a.cols+c]=std::exp(a.x[(size_t)r*a.cols+c]-mx));for(uint32_t c=0;c<a.cols;c++)t[i.b].x[(size_t)r*a.cols+c]/=s;}break;}
    case AIOp::RESULT:resultPtr=&at(i.a);break;
    default:throw Error("unknown AI opcode");
  }}}
};

// Framework-independent model container: architecture is AI bytecode and parameters
// are typed f32 constants held in the JUP AI section.
class JupModel {
public:
  std::vector<AIInstruction> architecture;
  std::vector<float> parameters;
  void run(JupAIRuntime& runtime) const { runtime.constants=parameters; runtime.run(architecture); }
};

struct Section { uint32_t type=0, flags=0; uint64_t offset=0,size=0; uint32_t crc=0; std::vector<uint8_t> data; };
class JupFile {
public:
  uint32_t flags=0,entry=0; uint64_t originalSize=0; std::vector<Instruction> program; std::vector<uint64_t> constants; std::vector<uint8_t> data; std::vector<AIInstruction> ai; std::vector<float> aiConstants; std::array<uint8_t,32> originalHash{};
  bool hasAI()const{return !ai.empty();}
  std::vector<uint8_t> serialize() const {
    std::vector<Section> s;
    s.push_back({S_PROG,0,0,0,0,encodeProgram(program)});
    std::vector<uint8_t> co;for(uint64_t n:constants)put<uint64_t>(co,n);s.push_back({S_CONS,0,0,0,0,co});
    s.push_back({S_DATA,0,0,0,0,data});
    std::vector<uint8_t> aa;put<uint32_t>(aa,1);put<uint32_t>(aa,(uint32_t)ai.size());put<uint32_t>(aa,(uint32_t)aiConstants.size());for(auto x:ai){put<uint8_t>(aa,(uint8_t)x.op);put<uint8_t>(aa,0);put<uint16_t>(aa,0);put<int32_t>(aa,x.a);put<int32_t>(aa,x.b);put<int32_t>(aa,x.c);}for(float f:aiConstants){uint32_t u;std::memcpy(&u,&f,4);put<uint32_t>(aa,u);}s.push_back({S_AI,0,0,0,0,aa});
    std::vector<uint8_t> me(originalHash.begin(),originalHash.end());put<uint64_t>(me,originalSize);put<uint32_t>(me,1);s.push_back({S_META,0,0,0,0,me});
    const uint64_t dir=64; uint64_t pos=dir+s.size()*32; for(auto& x:s){x.offset=pos;x.size=x.data.size();x.crc=crc32(x.data);pos+=x.size;}
    std::vector<uint8_t> out(64,0);out[0]='J';out[1]='U';out[2]='P';out[3]='1';put16(out,4,VERSION);out[6]=1;out[7]=64;put32at(out,8,flags|(hasAI()?FLAG_AI:0));put32at(out,12,entry);put32at(out,16,(uint32_t)program.size());put32at(out,20,(uint32_t)s.size());put64at(out,24,dir);put64at(out,32,pos+32);put64at(out,40,pos);put64at(out,48,originalSize);
    for(const auto& x:s){put<uint32_t>(out,x.type);put<uint32_t>(out,x.flags);put<uint64_t>(out,x.offset);put<uint64_t>(out,x.size);put<uint32_t>(out,x.crc);put<uint32_t>(out,0);}for(const auto& x:s)out.insert(out.end(),x.data.begin(),x.data.end());
    // The header CRC is part of the file-integrity digest, so establish it first.
    put32at(out,56,crc32(out.data(),56)); auto dig=sha256(out);out.insert(out.end(),dig.begin(),dig.end()); return out;
  }
  static void put16(std::vector<uint8_t>& v,size_t p,uint16_t n){v[p]=(uint8_t)n;v[p+1]=(uint8_t)(n>>8);}
  static JupFile parse(const std::vector<uint8_t>& b){
    if(b.size()<64+32)throw Error("file too short");if(std::string((char*)b.data(),4)!="JUP1")throw Error("not a JUP v1 file");size_t q=4;if(get<uint16_t>(b,q)!=VERSION||get<uint8_t>(b,q)!=1||get<uint8_t>(b,q)!=64)throw Error("unsupported JUP version or endian");
    q=8;JupFile f;f.flags=get<uint32_t>(b,q);f.entry=get<uint32_t>(b,q);uint32_t nins=get<uint32_t>(b,q), ns=get<uint32_t>(b,q);uint64_t dir=get<uint64_t>(b,q),fs=get<uint64_t>(b,q),integ=get<uint64_t>(b,q);f.originalSize=get<uint64_t>(b,q);uint32_t hc=get<uint32_t>(b,q);q+=4;
    if(nins>1000000||fs!=b.size()||integ+32!=b.size()||dir!=64||ns!=5||hc!=crc32(b.data(),56))throw Error("invalid JUP header");SHA256 hs;hs.update(b.data(),(size_t)integ);auto d=hs.finish();if(!std::equal(d.begin(),d.end(),b.begin()+integ))throw Error("JUP integrity SHA-256 mismatch");
    std::map<uint32_t,std::vector<uint8_t>> sec;std::vector<std::pair<uint64_t,uint64_t>> ranges;size_t dp=(size_t)dir;for(uint32_t i=0;i<ns;i++){uint32_t typ=get<uint32_t>(b,dp);get<uint32_t>(b,dp);uint64_t off=get<uint64_t>(b,dp),sz=get<uint64_t>(b,dp);uint32_t cr=get<uint32_t>(b,dp);get<uint32_t>(b,dp);if(off<64+ns*32||off>integ||sz>integ-off||sec.count(typ))throw Error("invalid section bounds");ranges.push_back({off,off+sz});std::vector<uint8_t>x(b.begin()+off,b.begin()+off+sz);if(crc32(x)!=cr)throw Error("section CRC mismatch");sec[typ]=x;}std::sort(ranges.begin(),ranges.end());for(size_t i=1;i<ranges.size();i++)if(ranges[i-1].second>ranges[i].first)throw Error("overlapping JUP sections");
    for(uint32_t typ:{S_PROG,S_CONS,S_DATA,S_AI,S_META})if(!sec.count(typ))throw Error("missing required section");f.program=decodeProgram(sec[S_PROG]);if(f.program.size()!=nins)throw Error("instruction count mismatch");if(sec[S_CONS].size()%8)throw Error("constant pool malformed");q=0;while(q<sec[S_CONS].size())f.constants.push_back(get<uint64_t>(sec[S_CONS],q));f.data=sec[S_DATA];
    const auto& m=sec[S_META];if(m.size()<44)throw Error("metadata malformed");std::copy(m.begin(),m.begin()+32,f.originalHash.begin());q=32;uint64_t ms=get<uint64_t>(m,q);if(ms!=f.originalSize)throw Error("metadata size mismatch");
    const auto& a=sec[S_AI];q=0;if(get<uint32_t>(a,q)!=1)throw Error("unsupported AI section version");uint32_t no=get<uint32_t>(a,q),nc=get<uint32_t>(a,q);if(no>JupAIRuntime::MAX_OPS||nc>JupAIRuntime::MAX_TENSOR_BYTES/4||a.size()!=12+(size_t)no*16+(size_t)nc*4)throw Error("AI section malformed or too large");for(uint32_t i=0;i<no;i++){AIInstruction z;z.op=(AIOp)get<uint8_t>(a,q);get<uint8_t>(a,q);get<uint16_t>(a,q);z.a=get<int32_t>(a,q);z.b=get<int32_t>(a,q);z.c=get<int32_t>(a,q);f.ai.push_back(z);}for(uint32_t i=0;i<nc;i++){uint32_t u=get<uint32_t>(a,q);float x;std::memcpy(&x,&u,4);f.aiConstants.push_back(x);}if(((f.flags&FLAG_AI)!=0)!=f.hasAI())throw Error("AI flag and section disagree");return f;
  }
};

class JupVM {
public:
  static constexpr size_t MAX_MEMORY=64ull*1024*1024, MAX_OUTPUT=512ull*1024*1024, MAX_STACK=1ull*1024*1024, MAX_STEPS=100000000;
  std::array<uint64_t,16> r{}; std::vector<uint64_t> stack,mem; std::vector<uint8_t> output; uint64_t steps=0,peakMemory=0,aiOps=0; const JupFile& f;
  explicit JupVM(const JupFile& x):f(x){}
  void emit(uint8_t v){if(output.size()>=MAX_OUTPUT)throw Error("output limit exceeded");output.push_back(v);peakMemory=std::max<uint64_t>(peakMemory,output.size()+mem.size()*8+stack.size()*8);}
  uint64_t val(int32_t x)const {if(x<0||x>=16)throw Error("register index out of range");return r[x];}
  void run(){if(f.entry>=f.program.size())throw Error("entry point out of program");size_t ip=f.entry;bool halted=false;while(!halted){if(++steps>MAX_STEPS)throw Error("instruction limit exceeded");if(ip>=f.program.size())throw Error("instruction pointer out of range");Instruction i=f.program[ip++];auto binary=[&](auto fn){if(i.a<0||i.a>=16||i.b<0||i.b>=16||i.c<0||i.c>=16)throw Error("register index out of range");r[i.a]=fn(r[i.b],r[i.c]);};switch(i.op){
      case Op::NOP:break;case Op::CONST:if(i.a<0||i.a>=16||i.b<0||(size_t)i.b>=f.constants.size())throw Error("bad CONST");r[i.a]=f.constants[i.b];break;
      case Op::PUSH:if(stack.size()>=MAX_STACK)throw Error("stack limit exceeded");stack.push_back(val(i.a));break;case Op::POP:if(stack.empty()||i.a<0||i.a>=16)throw Error("stack underflow");r[i.a]=stack.back();stack.pop_back();break;
      case Op::LOAD:if(i.a<0||i.a>=16||i.b<0||(size_t)i.b>=mem.size())throw Error("bad LOAD");r[i.a]=mem[i.b];break;case Op::STORE:if(i.a<0||(size_t)i.a>=MAX_MEMORY/8)throw Error("memory limit exceeded");if(mem.size()<=(size_t)i.a)mem.resize((size_t)i.a+1);mem[i.a]=val(i.b);break;
      case Op::ADD:binary([](uint64_t a,uint64_t b){return a+b;});break;case Op::SUB:binary([](uint64_t a,uint64_t b){return a-b;});break;case Op::MUL:binary([](uint64_t a,uint64_t b){return a*b;});break;case Op::DIV:binary([](uint64_t a,uint64_t b){if(!b)throw Error("division by zero");return a/b;});break;case Op::MOD:binary([](uint64_t a,uint64_t b){if(!b)throw Error("modulo by zero");return a%b;});break;
      case Op::AND:binary([](uint64_t a,uint64_t b){return a&b;});break;case Op::OR:binary([](uint64_t a,uint64_t b){return a|b;});break;case Op::XOR:binary([](uint64_t a,uint64_t b){return a^b;});break;case Op::NOT:if(i.a<0||i.b<0||i.a>=16||i.b>=16)throw Error("bad NOT");r[i.a]=~r[i.b];break;case Op::SHL:binary([](uint64_t a,uint64_t b){return a<<(b&63);});break;case Op::SHR:binary([](uint64_t a,uint64_t b){return a>>(b&63);});break;
      case Op::CMP:case Op::EQ:case Op::NE:case Op::LT:case Op::GT:case Op::LE:case Op::GE:{if(i.a<0||i.b<0||i.c<0||i.a>=16||i.b>=16||i.c>=16)throw Error("bad comparison");uint64_t a=r[i.b],b=r[i.c];r[i.a]=(i.op==Op::CMP?(a>b)-(a<b):i.op==Op::EQ?a==b:i.op==Op::NE?a!=b:i.op==Op::LT?a<b:i.op==Op::GT?a>b:i.op==Op::LE?a<=b:a>=b);break;}
      case Op::JUMP:if(i.a<0||(size_t)i.a>=f.program.size())throw Error("bad jump");ip=i.a;break;case Op::JZ:if(!val(i.a)){if(i.b<0||(size_t)i.b>=f.program.size())throw Error("bad jump");ip=i.b;}break;case Op::JNZ:if(val(i.a)){if(i.b<0||(size_t)i.b>=f.program.size())throw Error("bad jump");ip=i.b;}break;case Op::CALL:if(i.a<0||(size_t)i.a>=f.program.size()||stack.size()>=MAX_STACK)throw Error("bad call");stack.push_back(ip);ip=i.a;break;case Op::RET:if(stack.empty())throw Error("return stack underflow");ip=stack.back();stack.pop_back();break;
      case Op::COPY:if(i.a<0||i.b<0||(uint64_t)i.a+(uint64_t)i.b>f.data.size())throw Error("bad COPY range");for(int32_t k=0;k<i.b;k++)emit(f.data[i.a+k]);break;case Op::REPEAT:if(i.a<0||i.a>255||i.b<0)throw Error("bad REPEAT");for(int32_t k=0;k<i.b;k++)emit((uint8_t)i.a);break;case Op::RANGE:if(i.a<0||i.a>255||i.b<0)throw Error("bad RANGE");for(int32_t k=0;k<i.b;k++)emit((uint8_t)(i.a+k*i.c));break;case Op::WRITE:emit((uint8_t)val(i.a));break;
      case Op::AI_RUN:{if(!f.hasAI())throw Error("AI_RUN without AI section");JupModel model{f.ai,f.aiConstants};JupAIRuntime ai;model.run(ai);if(!ai.resultPtr)throw Error("AI program did not produce a result");aiOps+=ai.operations;for(float z:ai.resultPtr->x){uint32_t u;std::memcpy(&u,&z,4);for(int k=0;k<4;k++)emit((uint8_t)(u>>(8*k)));}break;}case Op::HALT:halted=true;break;default:throw Error("unknown opcode");
    }} }
};

class JupValidator { public: static void validate(const JupFile& f){if(f.program.empty())throw Error("empty program");for(auto i:f.program)if((uint8_t)i.op>(uint8_t)Op::HALT||(uint8_t)i.type>(uint8_t)Type::STRING)throw Error("unknown program opcode or type");if(f.hasAI()){JupModel model{f.ai,f.aiConstants};JupAIRuntime a;model.run(a);if(!a.resultPtr)throw Error("AI section has no result tensor");}} };
class JupEncoder {
public:
  static JupFile encode(const std::vector<uint8_t>& in){JupFile f;f.originalSize=in.size();f.originalHash=sha256(in);size_t i=0,litStart=0;auto flush=[&](size_t end){if(end>litStart){size_t n=end-litStart;if(n>std::numeric_limits<int32_t>::max())throw Error("input block too large");int32_t at=(int32_t)f.data.size();f.data.insert(f.data.end(),in.begin()+litStart,in.begin()+end);f.program.push_back({Op::COPY,Type::BYTES,0,at,(int32_t)n,0});}};while(i<in.size()){size_t rep=1;while(i+rep<in.size()&&in[i+rep]==in[i]&&rep<(size_t)std::numeric_limits<int32_t>::max())rep++;size_t ran=1;int step=0;if(i+1<in.size()){step=(int)(int8_t)(in[i+1]-in[i]);if(step!=0)while(i+ran<in.size()&&(uint8_t)(in[i]+ran*step)==in[i+ran]&&ran<(size_t)std::numeric_limits<int32_t>::max())ran++;}if(rep>=4||ran>=4){bool useRep=rep>=ran;flush(i);if(useRep)f.program.push_back({Op::REPEAT,Type::U8,0,in[i],(int32_t)rep,0}),i+=rep;else f.program.push_back({Op::RANGE,Type::U8,0,in[i],(int32_t)ran,step}),i+=ran;litStart=i;}else i++;}flush(in.size());f.program.push_back({Op::HALT});return f;}
  static JupFile aiDemo(){JupFile f;f.originalSize=16;f.flags=FLAG_AI;f.aiConstants={1,2,3,4, 5,6,7,8};f.ai={{AIOp::CREATE,0,2,2},{AIOp::CREATE,1,2,2},{AIOp::LOAD,0,0,4},{AIOp::LOAD,1,4,4},{AIOp::MATMUL,0,1,2},{AIOp::RELU,2,3,0},{AIOp::RESULT,3,0,0}};f.program={{Op::AI_RUN},{Op::HALT}};float values[]={19,22,43,50};std::vector<uint8_t> expected;for(float v:values){uint32_t u;std::memcpy(&u,&v,4);for(int k=0;k<4;k++)expected.push_back((uint8_t)(u>>(8*k)));}f.originalHash=sha256(expected);return f;}
};
class JupDecoder { public: static std::vector<uint8_t> run(const JupFile& f,JupVM* info=nullptr){if(info){info->run();return info->output;}JupVM v(f);v.run();return v.output;} };

static std::vector<uint8_t> readFile(const std::string& p){std::ifstream f(p,std::ios::binary);if(!f)throw Error("cannot open input: "+p);f.seekg(0,std::ios::end);std::streamoff n=f.tellg();if(n<0)throw Error("cannot read input size");if((uint64_t)n>512ull*1024*1024)throw Error("input exceeds 512 MiB safety limit");f.seekg(0);std::vector<uint8_t>x((size_t)n);if(n&&!f.read((char*)x.data(),n))throw Error("cannot read input");return x;}
static void writeFile(const std::string&p,const std::vector<uint8_t>&x){std::ofstream f(p,std::ios::binary|std::ios::trunc);if(!f)throw Error("cannot create output: "+p);if(!x.empty())f.write((const char*)x.data(),x.size());if(!f)throw Error("cannot write output");}
static std::string sizeText(uint64_t n){const char* u[]={"B","KiB","MiB","GiB"};double d=n;int i=0;while(d>=1024&&i<3){d/=1024;i++;}std::ostringstream o;o<<std::fixed<<std::setprecision(i?2:0)<<d<<' '<<u[i];return o.str();}
class JupCLI {
  static void info(const JupFile&f,const std::string&name){std::cout<<"JUP file: "<<name<<"\n  Version: 1\n  Program: "<<f.program.size()<<" instructions\n  Data: "<<sizeText(f.data.size())<<"\n  Original output: "<<sizeText(f.originalSize)<<"\n  AI: "<<(f.hasAI()?"enabled":"disabled")<<"\n";if(f.hasAI())std::cout<<"  AI program: "<<f.ai.size()<<" operations, "<<f.aiConstants.size()<<" constants\n";}
public:
  static int main(int argc,char**argv){try{if(argc<2)throw Error("usage: jup create|run|inspect|verify|benchmark|ai-info|ai-run|ai-demo|selftest ...");std::string c=argv[1];if(c=="selftest"){if(argc!=2)throw Error("usage: jup selftest");std::vector<std::vector<uint8_t>> cases={{},{'A','A','A','A','A','A','A'},{1,2,3,4,5,6,7,8},{9,4,9,4,3,2,1,0,255}};for(const auto& in:cases){JupFile encoded=JupEncoder::encode(in);JupFile parsed=JupFile::parse(encoded.serialize());JupValidator::validate(parsed);JupVM vm(parsed);vm.run();if(vm.output!=in||sha256(vm.output)!=parsed.originalHash)throw Error("self-test round trip failed");}JupFile demo=JupFile::parse(JupEncoder::aiDemo().serialize());JupValidator::validate(demo);JupVM av(demo);av.run();if(av.output.size()!=16||sha256(av.output)!=demo.originalHash)throw Error("self-test AI result failed");std::cout<<"Self-test passed: raw, REPEAT, RANGE, integrity, VM, SHA-256, and AI tensor execution.\n";return 0;}if(c=="create"){if(argc!=4)throw Error("usage: jup create <input> <output.jup>");auto start=std::chrono::steady_clock::now();auto in=readFile(argv[2]);JupFile j=JupEncoder::encode(in);auto bin=j.serialize();writeFile(argv[3],bin);auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();std::cout<<"Created "<<argv[3]<<"\n  Original: "<<sizeText(in.size())<<"\n  JUP: "<<sizeText(bin.size())<<"\n  Storage change: "<<std::fixed<<std::setprecision(2)<<(in.empty()?0.0:100.0*(1.0-(double)bin.size()/in.size()))<<"%\n  Generation: "<<ms<<" ms\n";return 0;}
    if(c=="ai-demo"){if(argc!=3)throw Error("usage: jup ai-demo <output.jup>");auto b=JupEncoder::aiDemo().serialize();writeFile(argv[2],b);std::cout<<"Created deterministic 2x2 AI demonstration: "<<argv[2]<<"\n";return 0;}
    if(c=="run"||c=="ai-run"){if(argc<3||argc>4)throw Error("usage: jup run <input.jup> <output> | jup ai-run <input.jup> [output]");auto b=readFile(argv[2]);JupFile f=JupFile::parse(b);JupValidator::validate(f);if(c=="ai-run"){if(!f.hasAI())throw Error("JUP file has no AI section");auto s=std::chrono::steady_clock::now();JupAIRuntime ai;ai.constants=f.aiConstants;ai.run(f.ai);if(!ai.resultPtr)throw Error("AI program did not produce a result");std::vector<uint8_t> out;for(float z:ai.resultPtr->x){uint32_t u;std::memcpy(&u,&z,4);for(int k=0;k<4;k++)out.push_back((uint8_t)(u>>(8*k)));}auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-s).count();if(argc==4)writeFile(argv[3],out);else {for(uint8_t x:out)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)x;std::cout<<std::dec<<"\n";}std::cout<<"AI execution:\n  Operations: "<<ai.operations<<"\n  Tensor memory: "<<sizeText(ai.memory)<<"\n  Output: "<<sizeText(out.size())<<"\n  Time: "<<std::fixed<<std::setprecision(3)<<ms<<" ms\n";return 0;}auto s=std::chrono::steady_clock::now();JupVM v(f);v.run();auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-s).count();if(argc!=4)throw Error("run requires an output path");writeFile(argv[3],v.output);std::cout<<"Execution:\n  Instructions: "<<v.steps<<"\n  AI operations: "<<v.aiOps<<"\n  Memory peak: "<<sizeText(v.peakMemory)<<"\n  Output: "<<sizeText(v.output.size())<<"\n  Time: "<<std::fixed<<std::setprecision(3)<<ms<<" ms\n";if(sha256(v.output)!=f.originalHash)throw Error("restored SHA-256 does not match metadata");return 0;}
    if(c=="inspect"||c=="verify"||c=="benchmark"||c=="ai-info"){if(argc!=3)throw Error("usage: jup "+c+" <input.jup>");auto b=readFile(argv[2]);JupFile f=JupFile::parse(b);if(c=="ai-info"){if(!f.hasAI())std::cout<<"AI: disabled\n";else std::cout<<"AI: enabled\n  Operations: "<<f.ai.size()<<"\n  Float constants: "<<f.aiConstants.size()<<"\n  Sandbox tensor memory limit: "<<sizeText(JupAIRuntime::MAX_TENSOR_BYTES)<<"\n";return 0;}JupValidator::validate(f);if(c=="inspect"){info(f,argv[2]);return 0;}auto s=std::chrono::steady_clock::now();JupVM v(f);v.run();auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-s).count();if(sha256(v.output)!=f.originalHash)throw Error("output SHA-256 mismatch");if(c=="verify")std::cout<<"Verified: structure, section CRCs, file SHA-256, VM execution, and output SHA-256 are valid.\n";else {info(f,argv[2]);std::cout<<"Benchmark:\n  File: "<<sizeText(b.size())<<"\n  Output: "<<sizeText(v.output.size())<<"\n  Runtime: "<<std::fixed<<std::setprecision(3)<<ms<<" ms\n  Instructions: "<<v.steps<<"\n  AI operations: "<<v.aiOps<<"\n  Memory peak: "<<sizeText(v.peakMemory)<<"\n";}return 0;}
    throw Error("unknown command: "+c);
  }catch(const std::exception&e){std::cerr<<"JUP error: "<<e.what()<<"\n";return 1;}}
};
} // namespace jup
int main(int argc,char**argv){return jup::JupCLI::main(argc,argv);}
