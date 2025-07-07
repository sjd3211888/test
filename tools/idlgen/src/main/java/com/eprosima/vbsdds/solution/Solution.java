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
// 2023-05 ~ 2025-05: idlgen optimization: support for pubsub mode java/jni code generation, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsdds.solution;

import com.eprosima.vbsdds.vbsddsgen;

import java.util.ArrayList;

public class Solution extends com.eprosima.solution.Solution {
    public Solution(vbsddsgen.LANGUAGE language, String example, String version, boolean serverside,
            boolean clientside) {
        super();

        m_publisherside = serverside;
        m_subscriberside = clientside;
        m_version = version;
        m_example = example;
        m_language = language;
    }

    public boolean getPublisherside() {
        return m_publisherside;
    }

    public boolean getSubscriberside() {
        return m_subscriberside;
    }

    @Override
    public ArrayList<String> getLibraries() {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for (int count = 0; count < libraries.size(); ++count) {
            if (m_example.contains("Win") && libraries.get(count).startsWith("vbsrtps"))
                ret.add(libraries.get(count) + "-" + m_version);
            else
                ret.add(libraries.get(count));
        }

        return ret;
    }

    public ArrayList<String> getLibrariesDebug() {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for (int count = 0; count < libraries.size(); ++count) {
            if (m_example.contains("Win") && libraries.get(count).startsWith("vbsrtps"))
                ret.add(libraries.get(count) + "d-" + m_version);
            else
                ret.add(libraries.get(count) + "d");
        }

        return ret;
    }

    public ArrayList<String> getLibrariesStatic() {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for (int count = 0; count < libraries.size(); ++count) {
            if (libraries.get(count).startsWith("ndds"))
                ret.add(libraries.get(count) + "z");
            else if (m_example.contains("Win") && libraries.get(count).startsWith("vbsrtps"))
                ret.add("lib" + libraries.get(count) + "-" + m_version);
            else
                ret.add("lib" + libraries.get(count));
        }

        return ret;
    }

    public ArrayList<String> getLibrariesStaticDebug() {
        ArrayList<String> libraries = super.getLibraries();
        ArrayList<String> ret = new ArrayList<String>();

        for (int count = 0; count < libraries.size(); ++count) {
            if (libraries.get(count).startsWith("ndds"))
                ret.add(libraries.get(count) + "zd");
            else if (m_example.contains("Win") && libraries.get(count).startsWith("vbsrtps"))
                ret.add("lib" + libraries.get(count) + "d-" + m_version);
            else
                ret.add("lib" + libraries.get(count) + "d");
        }

        return ret;
    }

    public boolean isIsJavaLanguage() {
        return m_language == vbsddsgen.LANGUAGE.JAVA;
    }

    public boolean isIsCPPAndJavaLanguage() {
        return m_language == vbsddsgen.LANGUAGE.CJ;
    }

    public boolean isIsCLanguage() {
        return m_language == vbsddsgen.LANGUAGE.C;
    }

    public boolean isIsRustLanguage() {
        return m_language == vbsddsgen.LANGUAGE.RUST;
    }

    public void setFileName(String onlyFileName) {
        m_onlyFileName = onlyFileName;
    }

    public String getFileName() {
        return m_onlyFileName;
    }

    private boolean m_publisherside = true;
    private boolean m_subscriberside = true;
    private String m_version = null;
    private String m_example = null;
    private vbsddsgen.LANGUAGE m_language = vbsddsgen.LANGUAGE.CPP; // Default language -> c++
    private String m_onlyFileName = "";
}
