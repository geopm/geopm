//  Copyright (c) 2015 - 2023, Intel Corporation
//  SPDX-License-Identifier: BSD-3-Clause
//

use std::sync::{Arc};
use tokio::sync::{Mutex};
use tonic::{Request, Response, Status};
use tonic::transport::{Server, Channel};
use tonic::transport::server::{UdsConnectInfo};
use geopm_package::geopm_service_server::{GeopmService, GeopmServiceServer};
use geopm_package::geopm_service_client::{GeopmServiceClient};
use geopm_package::{AccessLists, InfoRequest, SignalInfoList, ControlInfoList,
                    SessionKey, Empty, BatchRequest, BatchKey, BatchSession,
                    ReadRequest, Sample, WriteRequest, TopoCache};

pub mod geopm_package {
    tonic::include_proto!("geopm_package");
}


pub struct GeopmServiceImp {
    geopm_client: Arc<Mutex<GeopmServiceClient<Channel>>>,
}

fn session_key(
    conn_info: &UdsConnectInfo
) -> Result<SessionKey, Status> {
    let pid = conn_info.peer_cred.unwrap().pid().unwrap();
    let uid = conn_info.peer_cred.unwrap().uid();
    let name = format!("{},{}", uid, pid);
    Ok(SessionKey {name})
}

#[tonic::async_trait]
impl GeopmService for GeopmServiceImp {
    async fn open_session(
        &self,
        request: Request<SessionKey>,
    ) -> Result<Response<SessionKey>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let request = tonic::Request::new(session_key(conn_info).unwrap());
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.open_session(request).await?)
    }

    async fn close_session(
        &self,
        request: Request<SessionKey>,
    ) -> Result<Response<Empty>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let request = tonic::Request::new(session_key(conn_info).unwrap());
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.close_session(request).await?)
    }

    async fn get_user_access(
        &self,
        request: Request<SessionKey>,
    ) -> Result<Response<AccessLists>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let request = tonic::Request::new(session_key(conn_info).unwrap());
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.get_user_access(request).await?)
    }

    async fn get_signal_info(
        &self,
        request: Request<InfoRequest>,
    ) -> Result<Response<SignalInfoList>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let session_key = session_key(conn_info).unwrap();
        let mut info_request = request.into_inner();
        info_request.session_key = Some(session_key);
        let request = tonic::Request::new(info_request);
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.get_signal_info(request).await?)
    }

    async fn get_control_info(
        &self,
        request: Request<InfoRequest>,
    ) -> Result<Response<ControlInfoList>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let session_key = session_key(conn_info).unwrap();
        let mut info_request = request.into_inner();
        info_request.session_key = Some(session_key);
        let request = tonic::Request::new(info_request);
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.get_control_info(request).await?)
    }

    async fn start_batch(
        &self,
        request: Request<BatchRequest>,
    ) -> Result<Response<BatchKey>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let session_key = session_key(conn_info).unwrap();
        let mut batch_request = request.into_inner();
        batch_request.session_key = Some(session_key);
        let request = tonic::Request::new(batch_request);
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.start_batch(request).await?)
    }

    async fn stop_batch(
        &self,
        request: Request<BatchSession>,
    ) -> Result<Response<Empty>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let session_key = session_key(conn_info).unwrap();
        let mut batch_session = request.into_inner();
        batch_session.session_key = Some(session_key);
        let request = tonic::Request::new(batch_session);
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.stop_batch(request).await?)
    }

    async fn read_signal(
        &self,
        request: Request<ReadRequest>,
    ) -> Result<Response<Sample>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let session_key = session_key(conn_info).unwrap();
        let mut read_request = request.into_inner();
        read_request.session_key = Some(session_key);
        let request = tonic::Request::new(read_request);
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.read_signal(request).await?)
    }

    async fn write_control(
        &self,
        request: Request<WriteRequest>,
    ) -> Result<Response<Empty>, Status> {
        let conn_info = request.extensions().get::<UdsConnectInfo>().unwrap();
        let session_key = session_key(conn_info).unwrap();
        let mut write_request = request.into_inner();
        write_request.session_key = Some(session_key);
        let request = tonic::Request::new(write_request);
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.write_control(request).await?)
    }

    async fn topo_get_cache(
        &self,
        request: Request<Empty>,
    ) -> Result<Response<TopoCache>, Status> {
        let mut geopm_client = self.geopm_client.lock().await;
        Ok(geopm_client.topo_get_cache(request).await?)
    }
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let geopm_client = Arc::new(Mutex::new(GeopmServiceClient::connect("unix:///run/geopm-service/GRPC_SOCKET_PRIVATE").await?));

    let geopm_server = GeopmServiceImp {
        geopm_client,
    };
    Server::builder()
        .add_service(GeopmServiceServer::new(geopm_server))
        .serve("unix:///run/geopm-service/GRPC_SOCKET".parse().unwrap())
        .await?;
    Ok(())
}
