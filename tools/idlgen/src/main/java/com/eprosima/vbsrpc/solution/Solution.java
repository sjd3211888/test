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
// ------------------------------------------------------------------
// Modification history:
// 2023-08 ~ 2025-05: idlgen optimization: update of java rpc/dds cmake stg, rpc cmakefile fix, template file standardization, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsrpc.solution;

import java.util.ArrayList;
import com.eprosima.vbsrpc.vbsrpcgen;

public class Solution extends com.eprosima.solution.Solution
{
    public Solution(vbsrpcgen.PROTOCOL protocol, String example, boolean serverside, boolean clientside)
    {
        super();

        m_serverside = serverside;
        m_clientside = clientside;
        m_example = example;
	m_protocol = protocol;
    }

    public boolean getServerside()
    {
        return m_serverside;
    }

    public boolean getClientside()
    {
        return m_clientside;
    }

    public ArrayList<String> getLibrariesDebug()
    {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for(int count = 0; count < libraries.size(); ++count)
        {
            ret.add(libraries.get(count) + "d");
        }

        return ret;
    }

    public ArrayList<String> getLibrariesStatic()
    {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for(int count = 0; count < libraries.size(); ++count)
        {
            if(libraries.get(count).startsWith("ndds"))
                ret.add(libraries.get(count) + "z");
            else
                ret.add("lib" + libraries.get(count));
        }

        return ret;
    }

    public ArrayList<String> getLibrariesStaticDebug()
    {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for(int count = 0; count < libraries.size(); ++count)
        {
            if(libraries.get(count).startsWith("ndds"))
                ret.add(libraries.get(count) + "zd");
            else
                ret.add("lib" + libraries.get(count) + "d");
        }

        return ret;
    }

    public boolean isIsVS2015()
    {
	    return m_example.substring(8, 14).equals("VS2015");
    }

    public boolean isIsVS2013()
    {
	    return m_example.substring(8, 14).equals("VS2013");
    }

    public boolean isIsCj()
    {
	    return m_iscj;
    }

    public void setFileName(String onlyFileName) {
        m_onlyFileName = onlyFileName;
    }

    public String getFileName() {
        return m_onlyFileName;
    }

    public void setCjFlag(boolean cjFlag)
    {
        m_iscj = cjFlag;
    }

    private boolean m_serverside = true;
    private boolean m_clientside = true;
    private String m_example = null;
    private vbsrpcgen.PROTOCOL m_protocol = vbsrpcgen.PROTOCOL.VBSCDR; // Default protocol -> vbscdr
    private boolean m_iscj = false;
    private String m_onlyFileName = "";
}
