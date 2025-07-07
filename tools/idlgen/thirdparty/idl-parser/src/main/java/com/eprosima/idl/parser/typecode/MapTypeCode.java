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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting pubsub mode java/jni code generation, idl2xml parser, XML string interface, to_idl_string interface and fixes, adaptation to seq/array with nested bounded strings, length adjustment, xcdr, type validation, vbs idl to rti idl conversion, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import org.antlr.stringtemplate.StringTemplate;
import com.eprosima.idl.parser.tree.Definition;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;

public class MapTypeCode extends ContainerTypeCode
{
    public MapTypeCode(String maxsize, String valuesize)
    {
        super(Kind.KIND_MAP);
        m_maxsize = maxsize;
        m_valuesize = valuesize == null ? m_valuesize : valuesize;
    }

    @Override
    public String getTypeIdentifier()
    {
        return "TI_PLAIN_MAP_SMALL";
    }

    @Override
    public boolean isPlainType() { return true; }

    @Override
    public boolean isIsMapType() { return true; }

    @Override
    public boolean isIsType_19() { return true; }

    @Override
    public String getCppTypename()
    {
        StringTemplate st = getCppTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getCppTypename());
        st.setAttribute("value", getValueTypeCode().getCppTypename());
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getCTypename());
        st.setAttribute("value", getValueTypeCode().getCTypename());
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getCwrapperTypename()
    {
        return getCTypename() + "Wrapper";
    }

    @Override
    public String getJavaTypename()
    {
        StringTemplate st = getJavaTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getJavaWrapperTypename());
        st.setAttribute("value", getValueTypeCode().getJavaWrapperTypename());
        st.setAttribute("maxsize", String.valueOf(evaluateMathExpression(m_valuesize == null ? "100" : m_valuesize)));
        return st.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate st = getRustTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getRustTypename());
        st.setAttribute("value", getValueTypeCode().getRustTypename());
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getIdlTypename());
        st.setAttribute("value", getValueTypeCode().getIdlTypename());
        if (m_valuesize != null) {
            st.setAttribute("maxsize", String.valueOf(evaluateMathExpression(m_valuesize)));
        }
        return st.toString();
    }

    @Override
    public String getIdlFileTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getIdlFileTypename());
        st.setAttribute("value", getValueTypeCode().getIdlFileTypename());
        if (m_valuesize != null) {
            st.setAttribute("maxsize", String.valueOf(m_maxsize));
        }
        return st.toString();
    }

    @Override
    public String getXmlTypename()
    {
        StringTemplate st = getXmlTypenameFromStringTemplate();
        st.setAttribute("key", getKeyTypeCode().getXmlTypename());
        st.setAttribute("value", getValueTypeCode().getXmlTypename());
        st.setAttribute("maxsize", getMaxsize());
        return st.toString();
    }

    public String getMaxsize()
    {
        if(m_maxsize == null)
            return "100";

        return m_maxsize;
    }

    public boolean isMaxsizeGreaterThan1()
    {
        try {
            int maxsize = Integer.parseInt(getMaxsize());
            return maxsize > 1;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    public boolean isMaxsizeGreaterThan0()
    {
        try {
            int maxsize = Integer.parseInt(getMaxsize());
            return maxsize > 0;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    public String getXMLMaxsize()
    {
        if(m_valuesize == null)
            return "100";

        return String.valueOf(evaluateMathExpression(m_valuesize));
    }

    public TypeCode getKeyTypeCode()
    {
        return m_keyTypeCode;
    }

    public void setKeyTypeCode(TypeCode keyTypeCode)
    {
        m_keyTypeCode = keyTypeCode;
    }

    public TypeCode getValueTypeCode()
    {
        return m_valueTypeCode;
    }

    public void setValueTypeCode(TypeCode valueTypeCode)
    {
        m_valueTypeCode = valueTypeCode;
    }

    public Definition getKeyDefinition()
    {
        return m_keyDefinition;
    }

    public void setKeyDefinition(Definition keyDefinition)
    {
        m_keyDefinition = keyDefinition;
    }

    public Definition getValueDefinition()
    {
        return m_valueDefinition;
    }

    public void setValueDefinition(Definition valueDefinition)
    {
        m_valueDefinition = valueDefinition;
    }

    @Override
    public boolean isIsPlain()
    {
        return false;
    }

    @Override
    public boolean isIsBounded()
    {
        if (m_maxsize == null)
        {
            return false;
        }

        if (m_keyTypeCode != null && m_valueTypeCode != null)
        {
            return m_keyTypeCode.isIsBounded() && m_valueTypeCode.isIsBounded();
        }
        return false;
    }

    public boolean isUnbound()
    {
        return m_maxsize == null;
    }

    @Override
    public boolean isSetLength()
    {
        if(m_valuesize == null)
            return false;
        return true;
    }

//-------------maxSerializedSize----------------//
    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        long initial_alignment = current_alignment;
        long maxsize = Long.parseLong((m_valuesize == null ? "100" : String.valueOf(evaluateMathExpression(m_valuesize))), 10);

        if (!getValueTypeCode().isPrimitive() &&
            !getValueTypeCode().isIsType_c() /*enum*/)
        {
            // DHEADER if XCDRv2
            current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);
        }

        current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);

        if (0 < maxsize)
        {
            current_alignment += ((TypeCode)getKeyTypeCode()).maxSerializedSize(current_alignment);
            current_alignment += ((TypeCode)getValueTypeCode()).maxSerializedSize(current_alignment);

            if (1 < maxsize)
            {
                long element_size_after_first = ((TypeCode)getKeyTypeCode()).maxSerializedSize(current_alignment);
                element_size_after_first += ((TypeCode)getValueTypeCode()).maxSerializedSize(
                        current_alignment + element_size_after_first);
                current_alignment += element_size_after_first * (maxsize - 1);
            }
        }

        return current_alignment - initial_alignment;
    }

    @Override
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64) throws RuntimeGenerationException
    {
        throw new RuntimeGenerationException("MapTypeCode::maxPlainTypeSerializedSize(): Maps are not plain types.");
    }
//-------------maxSerializedSize----------------//

    @Override
    public String getHashstirng() {
        TypeCode tmp_key_true_TypeCode = m_keyTypeCode;
        TypeCode tmp_value_true_TypeCode = m_valueTypeCode;
        if(tmp_key_true_TypeCode instanceof AliasTypeCode) {
            tmp_key_true_TypeCode = ((AliasTypeCode)tmp_key_true_TypeCode).getTrueContentTypeCode();
        }
        if(tmp_value_true_TypeCode instanceof AliasTypeCode) {
            tmp_value_true_TypeCode = ((AliasTypeCode)tmp_value_true_TypeCode).getTrueContentTypeCode();
        }
        String key_hash_string = "", value_hash_string = "";

        if((tmp_key_true_TypeCode.isPrimitive() && !tmp_key_true_TypeCode.isIsType_c() && !tmp_key_true_TypeCode.isIsBitmaskType()) || tmp_key_true_TypeCode.isIsType_d() ||
            tmp_key_true_TypeCode.isIsType_f() || tmp_key_true_TypeCode.isIsSequenceType() || tmp_key_true_TypeCode.isIsMapType()) {
            //基础类型用名称
            key_hash_string = tmp_key_true_TypeCode.getHashstirng();
        } else {
            //复杂类型用hash
            key_hash_string = tmp_key_true_TypeCode.getHash();
        }
        if((tmp_value_true_TypeCode.isPrimitive() && !tmp_value_true_TypeCode.isIsType_c() && !tmp_value_true_TypeCode.isIsBitmaskType()) || tmp_value_true_TypeCode.isIsType_d() ||
            tmp_value_true_TypeCode.isIsType_f() || tmp_value_true_TypeCode.isIsSequenceType() || tmp_value_true_TypeCode.isIsMapType()) {
            //基础类型用名称
            value_hash_string = tmp_value_true_TypeCode.getHashstirng();
        } else {
            //复杂类型用hash
            value_hash_string = tmp_value_true_TypeCode.getHash();
        }
        return "map<" + key_hash_string + ", " + value_hash_string + (m_valuesize != null ? (", " + evaluateMathExpression(m_valuesize)) : "") + ">";
    }

    private TypeCode m_keyTypeCode = null;
    private TypeCode m_valueTypeCode = null;
    private Definition m_keyDefinition = null;
    private Definition m_valueDefinition = null;
    private String m_maxsize = null;
    private String m_valuesize = null;
}
