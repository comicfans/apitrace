#include "leaktracethread.h"

#include "trace_parser.hpp"
#include "globj_records.hpp"
#include "apitracecall.h"

void LeakTraceThread::run(){

    trace::Parser p;

    if (!p.open(filename.toLatin1().constData())) {
        //emit error
        //TODO
        return;
    }

    QList<ApiTraceError> errors;

    GLObjRecords records;

    records.traceLeaks(p,std::set<std::string>(),[&errors](
                std::string type ,unsigned int glName,
                std::shared_ptr<trace::Call> call){
            ApiTraceError error;

            error.callIndex=call->no;
            error.type=QString::fromStdString(type)+" Leak";
            error.message=
            "call number: "+QString::number(call->no)+
            " allocate "+QString::fromStdString(type)+
            ":"+QString::number(glName)+", but not deallocated it"; 

            errors.append(error);
            });


    error= !errors.isEmpty();
        
    emit leakTraceErrors(errors);

}

