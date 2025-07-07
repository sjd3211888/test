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

package com.eprosima.vbsrpc.idl.tree;

import java.security.MessageDigest;
import org.antlr.v4.runtime.Token;

public class Exception extends com.eprosima.idl.parser.tree.Exception
{
	public Exception(String scopeFile, boolean isInScope, String scope, String name, Token tk)
    {
        super(scopeFile, isInScope, scope, name, tk);
    }

    public String getMd5()
    {
        try
        {
            MessageDigest md = MessageDigest.getInstance("MD5");

            if(md != null)
            {
                byte[] md5 = md.digest(getScopedname().getBytes());
                int length = md5.length;
                return String.format("0x%02X%02X%02X%02X", md5[length - 4], md5[length - 3],
                        md5[length - 2], md5[length - 1]);
            }
        }
        catch(java.lang.Exception ex)
        {
            System.out.println("ERROR<Operation::getMd5>: " + ex.getMessage());
        }

        return null;
    }
}
