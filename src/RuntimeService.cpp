#include <grpcpp/grpcpp.h>
#include "geopm_runtime.grpc.pb.h"


namespace geopm {

    class RuntimeServiceImp final : public GEOPMRuntime::Service
    {
        public:
            RuntimeServiceImp() = default;
            virtual ~RuntimeServiceImp() = default;
            ::grpc::Status SetPolicy(::grpc::ServerContext* context, const ::Policy* request, ::Policy* response) override;
            ::grpc::Status GetReport(::grpc::ServerContext* context, const ::Empty* request, ::ReportList* response) override;
            ::grpc::Status AddChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response) override;
            ::grpc::Status RemoveChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response) override;
    };

    ::grpc::Status RuntimeServiceImp::SetPolicy(::grpc::ServerContext* context, const ::Policy* request, ::Policy* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::GetReport(::grpc::ServerContext* context, const ::Empty* request, ::ReportList* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::AddChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response)
    {
        ::grpc::Status result;
        return result;
    }

    ::grpc::Status RuntimeServiceImp::RemoveChildHost(::grpc::ServerContext* context, const ::Url* request, ::TimeSpec* response)
    {
        ::grpc::Status result;
        return result;
    }

}
