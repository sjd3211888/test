// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.eprosima.vbsrpc.idl.grammar;

import java.util.ArrayList;

import com.eprosima.vbsrpc.vbsrpcgen;

public class DDSContext extends Context
{
    public DDSContext(String filename, String file, ArrayList<String> includePaths, boolean clientcode, boolean servercode,
            String appProduct, boolean include_include_prefix, vbsrpcgen.DDS_TYPES ddstypes)
    {
        super(filename, file, includePaths, clientcode, servercode, appProduct, include_include_prefix);

        m_ddstypes = ddstypes;
    }

    public boolean isDds()
    {
        return true;
    }

    public boolean isCdr()
    {
        if(m_ddstypes == vbsrpcgen.DDS_TYPES.EPROSIMA)
            return true;

        return false;
    }

    public boolean isVbscdr()
    {
        return false;
    }

    public boolean isRtiTypes()
    {
        return m_ddstypes == vbsrpcgen.DDS_TYPES.RTI;
    }

    private vbsrpcgen.DDS_TYPES m_ddstypes = vbsrpcgen.DDS_TYPES.EPROSIMA;
}
