#pragma once
#include <drogon/HttpFilter.h>

class CORSFilter : public drogon::HttpFilter<CORSFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb,
                  drogon::FilterChainCallback &&fccb) override {
        // Just proceed with the request
        fccb();

        // The postHandlingAdvice will attach headers later
    }
};
