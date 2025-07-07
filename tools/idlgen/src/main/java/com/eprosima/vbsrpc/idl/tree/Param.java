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

import com.eprosima.idl.parser.typecode.TypeCode;
import com.eprosima.idl.parser.tree.Annotation;

public class Param extends com.eprosima.idl.parser.tree.Param
{
    public Param(String name, TypeCode typecode, Kind kind)
    {
        super(name, typecode, kind);
    }

    ////////// RESTful block //////////
    //

    /*
     * @brief Get the annotation with the real name of the query parameter.
     * Also substitute the spaces by %20.
     */
    public String getRealname()
    {
        Annotation annotation = ((Operation)getParent()).getAnnotations().get(getName());

        if(annotation != null)
        {

            String realname = annotation.getValue("value");

            if(realname != null)
            {
                return realname.replace(" ", "%20");
            }
        }

        return getName();
    }

    public boolean getBodyParam ()
    {
    	String bodyParam = ((Operation)getParent()).getBody();
        return getName().equals(bodyParam);
    }

    /////// End of RESTful block //////

}
