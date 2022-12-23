//  Copyright (c) 2015 - 2023, Intel Corporation
//  SPDX-License-Identifier: BSD-3-Clause
//

fn main() -> Result<(), Box<dyn std::error::Error>> {
    tonic_build::compile_protos("../geopm_service.proto")?;
    Ok(())
}
