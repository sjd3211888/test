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
// 2023-07 ~ 2025-05: Continuous enhancement of idlgen: cpp generation fix, support for tostring/get_type_name interfaces, enum as length assignment, typeobject fix, super large heap management, type validation, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;
import com.eprosima.idl.parser.tree.Annotation;

public class EnumMember extends Member
{
    public EnumMember(String name, String value)
    {
        super(null, name);
        m_value = value == null ? "" : value;
    }

    public EnumMember(EnumMember other)
    {
        super(other);
        m_value = other.getValue();
    }

    public boolean isHasValue() {
        return (getAnnotationValue() != null && !getAnnotationValue().isEmpty()) || !m_value.equals("");
    }

    public String getValue() {
        if (getAnnotationValue() != null && !getAnnotationValue().isEmpty()) {
            return getAnnotationValue();
        }
        if(m_value.matches("^0[xX][0-9A-Fa-f]+$")) {
            int intValue = Integer.parseInt(m_value.substring(2), 16);
            return String.valueOf(intValue);
        }

        return m_value;
    }

    public void setValue(String value) {
        if (value == null)
            return;
        m_value = value;
    }

    private String m_value;
}
