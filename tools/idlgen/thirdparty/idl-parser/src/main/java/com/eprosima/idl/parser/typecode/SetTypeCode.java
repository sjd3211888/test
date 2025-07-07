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
// 2023-05 ~ 2025-05: Enhancement of idlgen and idl2xml: support for idl2xml parser, XML string interface, xcdr, and adaptation to seq/array with nested bounded strings, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import org.antlr.stringtemplate.StringTemplate;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;

public class SetTypeCode extends ContainerTypeCode
{
    public SetTypeCode(String maxsize, String valuesize)
    {
        super(Kind.KIND_SET);
        m_maxsize = maxsize;
        m_valuesize = valuesize == null ? m_valuesize : valuesize;
    }

    @Override
    public boolean isIsType_e(){return true;}

    @Override
    public String getTypeIdentifier()
    {
        return "TI_PLAIN_SEQUENCE_SMALL";
    }

    @Override
    public boolean isPlainType() { return true; }

    @Override
    public boolean isIsSetType() { return true; }

    @Override
    public String getCppTypename()
    {
        StringTemplate st = getCppTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getCppTypename());
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getCTypename());
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
        st.setAttribute("type", getContentTypeCode().getJavaTypename());
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate st = getRustTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getRustTypename());
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getIdlTypename());
        st.setAttribute("maxsize", m_valuesize);
        return st.toString();
    }

    @Override
    public String getXmlTypename()
    {
        StringTemplate st = getXmlTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getXmlTypename());
        st.setAttribute("maxsize", m_maxsize);
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

        return super.isIsBounded();
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

        current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);

        if (0 < maxsize)
        {
            current_alignment += ((TypeCode)getContentTypeCode()).maxSerializedSize(current_alignment);

            if (1 < maxsize)
            {
                long element_size_after_first = ((TypeCode)getContentTypeCode()).maxSerializedSize(current_alignment);
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
        throw new RuntimeGenerationException("MapTypeCode::maxPlainTypeSerializedSize(): Sets are not plain types.");
    }
//-------------maxSerializedSize----------------//

    private String m_maxsize = null;
    private String m_valuesize = null;
}
