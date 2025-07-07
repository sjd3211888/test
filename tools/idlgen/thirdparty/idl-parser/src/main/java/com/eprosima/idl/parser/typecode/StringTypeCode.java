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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting pubsub mode java/jni code generation, idl2xml parser, XML string interface, RTI idl import and code generation, rpc build optimization, tostring and get_type_name interfaces, to_idl_string interface and improvement, xcdr support, wstring/wchar length adjustment, -hideInternals (structure member hiding) option, type validation, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import com.eprosima.idl.util.Pair;
import org.antlr.stringtemplate.StringTemplate;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;

public class StringTypeCode extends TypeCode
{
    public StringTypeCode(int kind, String maxsize, String valuesize, boolean isnotusevbsframework)
    {
        super(kind);
        m_maxsize = maxsize;
        m_valuesize = valuesize;
        m_isnotusevbs = isnotusevbsframework;
        useBBMode_ = false;
    }

    @Override
    public boolean isIsType_d(){return true;}

    @Override
    public boolean isIsType_15(){return getKind() == Kind.KIND_WSTRING;}

    @Override
    public String getTypeIdentifier()
    {
        switch(getKind())
        {
            case Kind.KIND_STRING:
                return "TI_STRING8_SMALL";
            case Kind.KIND_WSTRING:
                return "TI_STRING16_SMALL";
            default:
                return "TK_None";
        }
    }

    @Override
    public boolean isPlainType() { return true; }

    @Override
    public boolean isIsStringType() { return getKind() == Kind.KIND_STRING; }

    @Override
    public boolean isIsWStringType() { return getKind() == Kind.KIND_WSTRING; }

    @Override
    public String getCppTypename()
    {
        StringTemplate st = getCppTypenameFromStringTemplate();
        st.setAttribute("ctx", ctx);
        st.setAttribute("max_size", m_maxsize);
        st.setAttribute("isnotusevbs", m_isnotusevbs);
        return st.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("maxsize", getMaxsize());
        return st.toString();
    }

    @Override
    public String getCwrapperTypename()
    {
        return getCTypename()+"*";
    }

    @Override
    public String getJavaTypename()
    {
        return getJavaTypenameFromStringTemplate().toString();
    }

    @Override
    public String getRustTypename()
    {
        return getRustTypenameFromStringTemplate().toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("maxsize", getIDLMaxsize());
        return st.toString();
    }

    @Override
    public String getXmlTypename()
    {
        StringTemplate st = getXmlTypenameFromStringTemplate();
        return st.toString();
    }

    @Override
    public String getInitialValue()
    {
        return getInitialValueFromStringTemplate();
    }

    public String getMaxsize()
    {
        if(m_maxsize == null)
            return "255";

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

    public boolean isUnbound()
    {
        return m_maxsize == null;
    }

    public String getXMLMaxsize()
    {
        if(m_valuesize == null)
            return "255";

        return String.valueOf(evaluateMathExpression(m_valuesize));
    }

    public String getIDLMaxsize()
    {
        return m_valuesize == null ? null : String.valueOf(evaluateMathExpression(m_valuesize));
    }

    public int getMaxsizeInt()
    {
        if(m_valuesize == null)
            return 255;

        return evaluateMathExpression(m_valuesize);
    }

    public Pair<Integer, Integer> getMaxSerializedSize(int currentSize, int lastDataAligned)
    {
        int lcurrentSize = currentSize;

        // Length
        if(4 <= lastDataAligned)
        {
            lcurrentSize += 4;
        }
        else
        {
            int align = (4 - (lcurrentSize % 4)) & (4 - 1);
            lcurrentSize += 4 + align;
        }

        if(m_maxsize == null)
        {
            return new Pair<Integer, Integer>(lcurrentSize + 255 + 1, 1);
        }
        else
        {
            return new Pair<Integer, Integer>(lcurrentSize + Integer.parseInt(m_maxsize) + 1, 1);
        }
    }

    public int getMaxSerializedSizeWithoutAlignment(int currentSize)
    {
        if(m_maxsize == null)
        {
            return currentSize + 4 + 255 + 1;
        }
        else
        {
            return currentSize + 4 + Integer.parseInt(m_maxsize) + 1;
        }
    }

    @Override
    public boolean isIsPlain()
    {
        return false;
    }

    @Override
    public boolean isSetLength()
    {
        if(m_valuesize == null)
            return false;
        return true;
    }

    @Override
    public boolean isIsBounded()
    {
        return (m_maxsize != null);
    }

    @Override
    public String getSize()
    {
      return getXMLMaxsize();
    }

    @Override
    public String getEstimateSize()
    {
        return getXMLMaxsize();
    }

    @Override
    public void setUseBBMode(boolean useBBMode) {
        useBBMode_ = useBBMode;
    }

    @Override
    public boolean isUseBBMode() {
        return useBBMode_;
    }

//-------------maxSerializedSize----------------//
    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        long initial_alignment = current_alignment;
        long maxsize = Long.parseLong((m_valuesize == null ? "255" : String.valueOf(evaluateMathExpression(m_valuesize))), 10);

        switch (getKind())
        {
            case Kind.KIND_STRING:
                current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4) + maxsize + 1;
                break;
            case Kind.KIND_WSTRING:
                current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4) + (maxsize * 4);
                break;
        }

        return current_alignment - initial_alignment;
    }

    @Override
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64) throws RuntimeGenerationException
    {
        if ((ctx.isZeroCopy() && m_maxsize != null) || (ctx.isUseStaticCapacity())) {
            return maxSerializedSize(current_alignment);
        } else {
            throw new RuntimeGenerationException("StringTypeCode::maxPlainTypeSerializedSize(): Strings are not plain types.");
        }
    }
//-------------maxSerializedSize----------------//

    @Override
    public String getHashstirng() {
        return getIdlTypename();
    }

    private String // `m_maxsize` is a private member variable in the `StringTypeCode` class in Java.
    // It is used to store the maximum size of the string type being represented by an
    // instance of `StringTypeCode`. The value of `m_maxsize` is set through the
    // constructor when creating an instance of `StringTypeCode`, and it can be accessed
    // through various methods within the class to retrieve or manipulate the maximum
    // size information related to the string type.
    m_maxsize = null;
    private String m_valuesize = null;
    private boolean m_isnotusevbs = true;
    private boolean useBBMode_;
}
