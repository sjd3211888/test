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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting pubsub mode java/jni code generation, idl2xml parser, XML string interface, XML parsing content adjustment, xcdr, to_idl_string interface and fixes, adaptation to seq/array with nested bounded strings and default values, zero copy support for variable length seq, -hideInternals option for structure member hiding, StaticCapacitySeq-related structure fix, vbs idl to rti idl conversion, type validation, and abi compatibility optimization, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import com.eprosima.idl.parser.exception.RuntimeGenerationException;
import com.eprosima.idl.parser.typecode.StringTypeCode;
import com.eprosima.idl.parser.typecode.MapTypeCode;
import org.antlr.stringtemplate.StringTemplate;
import java.util.List;
import java.util.ArrayList;

public class SequenceTypeCode extends ContainerTypeCode
{
    public SequenceTypeCode(String maxsize, String valuesize)
    {
        super(Kind.KIND_SEQUENCE);
        m_maxsize = maxsize;
        m_valuesize = valuesize == null ? m_valuesize : valuesize;
        useBBMode_ = false;
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
    public boolean isIsSequenceType() { return true; }

    @Override
    public String getCppTypename()
    {
        StringTemplate st = getCppTypenameFromStringTemplate();
        st.setAttribute("ctx", ctx);
        st.setAttribute("type", getContentTypeCode().getCppTypename());
        String contenttype = getContentTypeCode().getCppTypename().replaceAll("::", "_");
        if(getContentTypeCode() instanceof StringTypeCode)
        {
            contenttype = contenttype.replace("*", "_ptr_") + ((StringTypeCode)getContentTypeCode()).getMaxsize();
        }
        st.setAttribute("contenttype", contenttype);
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    public String getBBCppTypename()
    {
        if(isUnbound())
            return getCppTypename();

        List<String> tmp_list = new ArrayList<String>();
        tmp_list = getContentTypeCodeDimensions();

        StringTemplate first = null, second = null, fin = null;
        String prevf = null, prevs = null;

        for(int count = 0; count < tmp_list.size(); ++count)
        {
            first = cpptypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_first");
            second = cpptypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_second");
            second.setAttribute("size", tmp_list.get(count));

            if(prevf != null)
            {
                first.setAttribute("prev", prevf);
            }
            if(prevs != null)
            {
                second.setAttribute("prev", prevs);
            }

            prevf = first.toString();
            prevs = second.toString();
        }
        fin = cpptypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY));;
        fin.setAttribute("firs", prevf);
        fin.setAttribute("secon", prevs);
        fin.setAttribute("type", getContentTypeCode().getCppTypename());

        return fin.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("ctx", ctx);
        st.setAttribute("type", getContentTypeCode().getCTypename());
        String contenttype = getContentTypeCode().getCTypename().replaceAll("::", "_");
        if(getContentTypeCode() instanceof StringTypeCode)
        {
            contenttype = contenttype.replace("*", "_ptr_") + ((StringTypeCode)getContentTypeCode()).getMaxsize();
        }
        st.setAttribute("contenttype", contenttype);
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getCwrapperTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("ctx", ctx);
        st.setAttribute("type", getContentTypeCode().getCwrapperTypename());
        TypeCode contentType = getContentTypeCode();

        while (contentType instanceof AliasTypeCode) {
            contentType = ((AliasTypeCode)contentType).getTrueContentTypeCode();
        }

        String contenttype = contentType.getCwrapperTypename().replaceAll("::", "_");

        if(contentType instanceof StringTypeCode)
        {
            contenttype = contenttype.replace("*", "_ptr_") + ((StringTypeCode)getContentTypeCode()).getMaxsize();
            st.setAttribute("contenttype", contenttype);
            st.setAttribute("maxsize", m_maxsize);
            return st.toString()+"*";
        }
        if(contentType instanceof EnumTypeCode){
            return st.toString()+"*";
        } else if(contentType instanceof BitmaskTypeCode){
            return st.toString()+"*";
        } else if(contentType instanceof PrimitiveTypeCode){
            return st.toString()+"*";
        } else if(contentType instanceof StructTypeCode){
            return st.toString()+"*";
        } else {
        return st.toString();
        }
    }

    public String getCTypeDimensions()
    {
        String dimensions = "[" + getMaxsize()  + "]";
        if(getContentTypeCode() instanceof StringTypeCode)
        {
            dimensions += "[" + ((StringTypeCode)getContentTypeCode()).getMaxsize() + "]";
        }

        return dimensions;
    }

    @Override
    public String getJavaTypename()
    {
        StringTemplate st = getJavaTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getJavaWrapperTypename());
        st.setAttribute("maxsize", String.valueOf(evaluateMathExpression(m_valuesize == null ? "10" : m_valuesize)));
        return st.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate st = getRustTypenameFromStringTemplate();
        st.setAttribute("ctx", ctx);
        st.setAttribute("type", getContentTypeCode().getRustTypename());
        String contenttype = getContentTypeCode().getRustTypename().replaceAll("::", "_");
        if(getContentTypeCode() instanceof StringTypeCode)
        {
            contenttype = contenttype.replace("*", "_ptr_") + ((StringTypeCode)getContentTypeCode()).getMaxsize();
        }
        st.setAttribute("contenttype", contenttype);
        st.setAttribute("maxsize", m_maxsize);
        return st.toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getIdlTypename());
        if (m_valuesize != null) {
            st.setAttribute("maxsize", String.valueOf(evaluateMathExpression(m_valuesize)));
        }
        return st.toString();
    }

    @Override
    public String getIdlFileTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("type", getContentTypeCode().getIdlFileTypename());
        if (m_valuesize != null) {
            st.setAttribute("maxsize", String.valueOf(evaluateMathExpression(m_valuesize)));
        }
        return st.toString();
    }

    @Override
    public String getXmlTypename()
    {
        StringTemplate st = getXmlTypenameFromStringTemplate();
        String tmp_type = getContentTypeCode().getXmlTypename();
        if(!getContentTypeCode().isUnbound()) {
            tmp_type += getContentTypeCode().getMaxsize();
        }
        st.setAttribute("type", tmp_type);
        if (m_valuesize != null) {
            st.setAttribute("maxsize", getMaxsize());
        }
        return st.toString();
    }

    public String getMaxsize()
    {
        if(m_maxsize == null)
            return "10";

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
            return "10";

        return String.valueOf(evaluateMathExpression(m_valuesize));
    }

    public boolean isUnbound()
    {
        return m_maxsize == null;
    }

    @Override
    public boolean isIsPlain()
    {
        if (ctx.isUseStaticCapacity()) {
            return true;
        }

        if (ctx.isZeroCopy() && m_maxsize != null) {
            return true;
        }
        return false;
    }

    @Override
    public boolean isIsBounded()
    {
        if (ctx.isUseStaticCapacity()) {
            return true;
        }

        if (m_maxsize == null)
        {
            return false;
        }

        return super.isIsBounded();
    }

    public boolean CheckContentTypeCodeIfUseBB()
    {
        TypeCode tmp_code = getContentTypeCode();

        if(tmp_code.isIsSequenceType()) {
            return ((SequenceTypeCode)tmp_code).CheckContentTypeCodeIfUseBB();
        } else {
            if(!tmp_code.isIsPlain()) {
                return false;
            }
        }
        return false;
    }

    public List<String> getContentTypeCodeDimensions()
    {
        TypeCode tmp_code = getContentTypeCode();
        List<String> tmp_list = new ArrayList<String>();
        tmp_list.add(getMaxsize());
        while (tmp_code.isIsSequenceType()) {
            tmp_list.add(((SequenceTypeCode)tmp_code).getMaxsize());
            tmp_code = tmp_code.getContentTypeCode();
        }
        return tmp_list;
    }

    @Override
    public String getEstimateSize()
    {
        int tmpSize = evaluateMathExpression(m_valuesize == null ? "0": m_valuesize);
        return String.valueOf(tmpSize * evaluateMathExpression(getContentTypeCode().getEstimateSize() == null ? "0" : getContentTypeCode().getEstimateSize()));
    }

    public void setUseBBMode(boolean useBBMode) {
        useBBMode_ = useBBMode;
    }

    @Override
    public boolean isUseBBMode() {
        return useBBMode_;
    }

    @Override
    public boolean isSetLength()
    {
        if(getContentTypeCode() != null && !getContentTypeCode().isSetLength()) {
            return false;
        }
        return true;
    }

    public List<String> getDimensionsTrueValue()
    {
        List<String> true_dimensions = new ArrayList<String>();
        true_dimensions.add(m_valuesize);
        return true_dimensions;
    }

//-------------maxSerializedSize----------------//
    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        long initial_alignment = current_alignment;
        long maxsize = !detect_recursive_
            ? Long.parseLong((m_valuesize == null ? (ctx.getStaticCapacity() != 0 ? String.valueOf(ctx.getStaticCapacity()) : "10") : String.valueOf(evaluateMathExpression(m_valuesize))), 10)
            : 0;

        boolean should_set_and_unset = !detect_recursive_;

        if (should_set_and_unset)
        {
            detect_recursive_ = true;
        }

        if (!getContentTypeCode().isPrimitive() &&
            !getContentTypeCode().isIsType_c() /*enum*/)
        {
            // DHEADER if XCDRv2
            current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);
        }

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

        if (should_set_and_unset)
        {
            detect_recursive_ = false;
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
            throw new RuntimeGenerationException("MapTypeCode::maxPlainTypeSerializedSize(): Sequences are not plain types.");
        }
    }
//-------------maxSerializedSize----------------//
    @Override
    public String getHashstirng() {
        TypeCode tmp_true_TypeCode = getTrueContentTypeCode();
        if(tmp_true_TypeCode instanceof AliasTypeCode) {
            tmp_true_TypeCode = ((AliasTypeCode)tmp_true_TypeCode).getTrueContentTypeCode();
        }
        String inner_hash_string = "";

        if((tmp_true_TypeCode.isPrimitive() && !tmp_true_TypeCode.isIsType_c() && !tmp_true_TypeCode.isIsBitmaskType()) || tmp_true_TypeCode.isIsType_d() ||
            tmp_true_TypeCode.isIsType_f() || tmp_true_TypeCode.isIsSequenceType() || tmp_true_TypeCode.isIsMapType()) {
            //基础类型用名称
            inner_hash_string = tmp_true_TypeCode.getHashstirng();
        } else {
            //复杂类型用hash
            inner_hash_string = tmp_true_TypeCode.getHash();
        }
        return "sequence<" + inner_hash_string + (m_valuesize != null ? (", " + evaluateMathExpression(m_valuesize)) : "") + ">";
    }

    private String m_maxsize = null;
    private String m_valuesize = null;
    private boolean useBBMode_;
    protected boolean detect_recursive_ = false;
}
