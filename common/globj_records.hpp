/*
 * =====================================================================================
 *
 *       Filename:  globj_records.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/19/2016 02:59:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#pragma once

#include "trace_model.hpp"

#include <set>
#include <memory>
#include <functional>

namespace trace{
    class Parser;
}

class GLObjRecords{

    public:

        std::string normalize(const std::string& tobeNormalized);

                //thread id,        // type tag                          // glName,       // generate call
        
        typedef std::function<void(std::string ,unsigned int ,
                std::shared_ptr<trace::Call>)> LeakCallback;

        void traceLeaks(trace::Parser& p,
                std::set<std::string> catalogs,LeakCallback leakCallback);

    private:

        void generated(std::shared_ptr<trace::Call> call);

        void deleted(const trace::Call & call);

        typedef std::map<unsigned int,std::shared_ptr<trace::Call> > NameMap;
        typedef std::map<std::string,NameMap> ThreadRecord;

        typedef std::map<int,ThreadRecord> RecordsType;
        RecordsType records;
};

