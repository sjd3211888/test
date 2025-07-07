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
// 2023-05 ~ 2025-05: idlgen optimization: support for pubsub mode java/jni code generation, gerrit adaption, code cleanup, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsrpc.idl.grammar;

import java.util.ArrayList;

public class VBSContext extends Context {
    public VBSContext(String filename, String file, ArrayList<String> includePaths, boolean clientcode,
            boolean servercode,
            String appProduct, boolean include_include_prefix, boolean isStream) {
        super(filename, file, includePaths, clientcode, servercode, appProduct, include_include_prefix);
        m_isStream = isStream;
    }

    public boolean isDds() {
        return false;
    }

    public boolean isCdr() {
        return true;
    }

    public boolean isVbscdr() {
        return false;
    }

    public boolean isRtiTypes() {
        return false;
    }

    public boolean isStreamMode() {
        return m_isStream;
    }

    private boolean m_isStream = false;
}
