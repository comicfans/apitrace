/*
 * =====================================================================================
 *
 *       Filename:  globj_records.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/19/2016 03:00:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:   comicfans44, comicfans44@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include "globj_records.hpp"
#include "trace_parser.hpp"

#include <algorithm>

class TypeVisitor: public trace::Visitor{

    public:

        TypeVisitor(){}

        virtual void visit(trace::UInt * v){

            isUint=true;
            uintValue = v->toUInt();
        }

        virtual void visit(trace::Array *v){

            isArray=true;
            array=v;
        }
        
        bool isUint=false;
        bool isArray=false;

        trace::Array *array;
        unsigned int uintValue;
};

static bool
isSupported(const trace::Call &call,TypeVisitor &v){

    if(call.sig->num_args!=2){
        //not support
        return false;
    }

    call.args[0].value->visit(v);

    if(!v.isUint){

        return false;
    }

    call.args[1].value->visit(v);

    if(!v.isArray){
        return false;
    }

    return true;
}

void GLObjRecords::generated(std::shared_ptr<trace::Call> call){

    TypeVisitor v;

    if (!isSupported(*call,v)) {
        return;
    }


    std::string normalizedCallName = GLObjRecords::normalize(call->sig->name);

    std::string keyOnly = normalizedCallName.substr(sizeof("glGen")-1);

    NameMap& nameMap = records[call->thread_id][keyOnly];

    for(auto &s: v.array->values){

        auto glName=s->toUInt();


        auto shouldBeEnd = nameMap.find(glName);

        assert(shouldBeEnd==nameMap.end());

        nameMap.emplace(std::make_pair(glName,call));
    }
}

void GLObjRecords::deleted(const trace::Call &call){

    TypeVisitor v;

    if (!isSupported(call,v)) {
        return;
    }

    std::string normalizedCallName = GLObjRecords::normalize(call.sig->name);

    std::string keyOnly = normalizedCallName.substr(sizeof("glDelete")-1);

    auto& threadRecord = records[call.thread_id];

    auto& nameMap = threadRecord[keyOnly];

    for(int i=0;i<v.uintValue;++i){

        auto glName=(*v.array)[i].toUInt();

        auto shouldNotEnd = nameMap.find(glName);

        if(shouldNotEnd!=nameMap.end()){
            nameMap.erase(shouldNotEnd);
            continue;
        }

        std::cerr<<"wrong type delete called"<<keyOnly<<":"<<glName<<'\n';
        //this is delete wrong type object error
    }

    if (nameMap.empty()) {
        threadRecord.erase(threadRecord.find(keyOnly));
    }

}

std::string GLObjRecords::normalize(const std::string& toBeNormalized){

    static std::map<std::string,std::string> NORMALIZED_MAP;
  
    static const bool static_init = [&]()->bool{

        for(auto call: {"Textures","Buffers","FrameBuffers","RenderBuffers"}){

            for(auto ext: {"EXT","ARB"}){
                auto normalizedGenCall=std::string("glGen")+call;
                auto normalizedDeleteCall=std::string("glDelete")+call;
                NORMALIZED_MAP[normalizedGenCall+ext]=normalizedGenCall;
                NORMALIZED_MAP[normalizedDeleteCall+ext]=normalizedDeleteCall;
            }
        }
        return true;
    }();


    auto it=NORMALIZED_MAP.find(toBeNormalized);
    if (it!=NORMALIZED_MAP.end()) {
        return it->second;
    }

    return toBeNormalized;
}

void 
GLObjRecords::traceLeaks(trace::Parser& p,std::set<std::string> catalogs,LeakCallback leakCallback){

    if (catalogs.empty()) {
        
        catalogs.insert("Textures");
        catalogs.insert("Buffers");
        catalogs.insert("RenderBuffers");
        catalogs.insert("FrameBuffers");
        catalogs.insert("VertexArrays");
    }

    std::vector<std::string> genMatches;
    std::vector<std::string> deleteMatches;

    for(auto catalog:catalogs){
        genMatches.push_back("glGen"+catalog);
        deleteMatches.push_back("glDelete"+catalog);
    }

    trace::Call *call;
    while ((call = p.parse_call())) {

        auto name=call->sig->name;

        std::shared_ptr<trace::Call> callPtr(call);

        bool matched=false;
        for(auto genMatch:genMatches){
            if (std::string(name).find(genMatch)!=std::string::npos) {
                generated(callPtr);
                matched=true;
            }
        }

        if (!matched) {
            for(auto deleteMatch:deleteMatches){
                if (std::string(name).find(deleteMatch)!=std::string::npos) {
                    deleted(*call);
                }
            }    
        }

    }

    for(auto threadLeaks:records){


        for(auto typeValues:threadLeaks.second){

            std::vector<std::pair<unsigned int,std::shared_ptr<trace::Call> > > reordered;
            for(auto glNameAndCall: typeValues.second){
                reordered.push_back(glNameAndCall);
            }

            std::sort(reordered.begin(),reordered.end(),
                    [](const std::pair<unsigned int,std::shared_ptr<trace::Call> >& lhs,
                        const std::pair<unsigned int,std::shared_ptr<trace::Call> >& rhs){
                    return lhs.second->no<rhs.second->no;
                    });

            for(auto ordered: reordered){
                leakCallback(typeValues.first,ordered.first,ordered.second);
            }
        }
    }
}
