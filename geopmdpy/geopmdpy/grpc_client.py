#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import grpc
from geopmdpy import geopm_service_pb2, geopm_service_pb2_grpc

class GRPCClient:
    """A client for interacting with the GEOPM gRPC service."""

    def __init__(self, address="/run/geopm/grpc.sock"):
        """Initialize the GRPCClient with the gRPC server address.

        Args:
            address (str): The path to the gRPC UDS (default: /run/geopm/grpc.sock).
        """
        credentials = grpc.local_channel_credentials(grpc.LocalConnectionType.UDS)
        self.channel = grpc.secure_channel(f'unix://{address}', credentials)
        self.stub = geopm_service_pb2_grpc.GEOPMServiceStub(self.channel)

    def PlatformGetGroupAccess(self, group):
        """Get the signal and control access lists for a group.

        Args:
            group (str): The Unix group name.

        Returns:
            tuple: A tuple containing lists of allowed signals and controls.
        """
        request = geopm_service_pb2.GroupAccessRequest(group=group)
        response = self.stub.GetGroupAccess(request)
        return response.signals, response.controls

    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls):
        """Set the signal and control access lists for a group.

        Args:
            group (str): The Unix group name.
            allowed_signals (list): List of allowed signal names.
            allowed_controls (list): List of allowed control names.
        """
        request = geopm_service_pb2.GroupAccessRequest(
            group=group,
            allowed_signals=allowed_signals,
            allowed_controls=allowed_controls
        )
        self.stub.SetGroupAccess(request)

    def PlatformSetGroupAccessSignals(self, group, allowed_signals):
        """Set the signal access list for a group.

        Args:
            group (str): The Unix group name.
            allowed_signals (list): List of allowed signal names.
        """
        request = geopm_service_pb2.GroupAccessRequest(
            group=group,
            allowed_signals=allowed_signals
        )
        self.stub.SetGroupAccessSignals(request)

    def PlatformSetGroupAccessControls(self, group, allowed_controls):
        """Set the control access list for a group.

        Args:
            group (str): The Unix group name.
            allowed_controls (list): List of allowed control names.
        """
        request = geopm_service_pb2.GroupAccessRequest(
            group=group,
            allowed_controls=allowed_controls
        )
        self.stub.SetGroupAccessControls(request)

    def PlatformGetAllAccess(self):
        """Get all supported signals and controls.

        Returns:
            tuple: A tuple containing lists of all supported signals and controls.
        """
        request = geopm_service_pb2.Empty()
        response = self.stub.GetAllAccess(request)
        return response.signals, response.controls

    def PlatformGetUserAccess(self):
        """Get the user-specific allowed signals and controls.

        Returns:
            tuple: A tuple containing lists of user-allowed signals and controls.
        """
        request = geopm_service_pb2.SessionKey(name="")
        response = self.stub.GetUserAccess(request)
        return response.signals, response.controls

    def PlatformGetGroupAccess(self, group):
        """Get the signal and control access lists for a group.

        Args:
            group (str): The Unix group ID.

        Returns:
            tuple: A tuple containing lists of allowed signals and controls.
        """
        request = geopm_service_pb2.GroupAccessRequest(group=group)
        response = self.stub.GetGroupAccess(request)
        return response.signals, response.controls

    def PlatformStartProfile(self, profile_name):
        """Start profiling for the calling process

        """
        request = geopm_service_pb2.ProfileRequest(profile_name=profile_name)
        self.stub.StartProfile(request)

    def PlatformStopProfile(self, region_names):
        """Stop profiling for the calling process

        """
        request = geopm_service_pb2.ProfileRequest(region_names=region_names)
        self.stub.StopProfile(request)
