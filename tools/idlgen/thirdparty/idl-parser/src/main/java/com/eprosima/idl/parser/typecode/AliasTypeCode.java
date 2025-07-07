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
// 2023-05 ~ 2025-05: idlgen and idl2xml optimization: support for idl2xml parser, java typedef, tostring/get_type_name, xcdr, multi-level nested typedef, array fixes and improvements, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import com.eprosima.idl.parser.exception.RuntimeGenerationException;
import com.eprosima.idl.util.Pair;
import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;
import org.antlr.stringtemplate.StringTemplate;

public class AliasTypeCode extends ContainerTypeCode
{
    public AliasTypeCode(String scope, String name)
    {
        super(Kind.KIND_ALIAS);
        int index = name.lastIndexOf("::");
        if (index != -1) {
            m_scope_normal = (scope.equals("") ? "" : scope + "::") + name.substring(0, index);
            m_scope = (m_scope_normal.equals("") ? "" : "::" + (scope.equals("") ? "" : scope + "::")) + name.substring(0, index);
            m_name = name.substring(index+2);
        } else {
            m_scope = scope.equals("") ? "" : "::" + scope;
            m_scope_normal = scope;
            m_name = name;
        }
        m_name_origin = name;
    }

    public AliasTypeCode(AliasTypeCode other)
    {
        super(other);
        this.m_scope = other.getScope();
        this.m_scope_normal = other.getAliasScopeNormal();
        this.m_name = other.getName();
        this.m_name_origin = other.getNameOrigin();
    }

    //打穿所有alias和内嵌类型，获取最内类型，如果内嵌是alias不会被击穿
    @Override
    public TypeCode getContentTypeCode()
    {
        if(super.getContentTypeCode() instanceof AliasTypeCode)
        {
            AliasTypeCode alias = (AliasTypeCode)super.getContentTypeCode();
            return alias.getContentTypeCode();
        }
        else if(super.getContentTypeCode() instanceof ContainerTypeCode)
        {
            ContainerTypeCode container = (ContainerTypeCode)super.getContentTypeCode();
            return container.getContentTypeCode();
        }

        return super.getContentTypeCode();
    }

    //打穿所有alias，取真实的第一层内嵌类型
    @Override
    public TypeCode getTrueContentTypeCode()
    {
        if(super.getTrueContentTypeCode() instanceof AliasTypeCode)
        {
            AliasTypeCode alias = (AliasTypeCode)super.getTrueContentTypeCode();
            return alias.getTrueContentTypeCode();
        }
        return super.getTrueContentTypeCode();
    }

    //打穿所有alias和内嵌类型，获取最内类型，alias会被击穿
    public TypeCode getDeepestContentTypeCode()
    {
        if(super.getContentTypeCode() instanceof AliasTypeCode)
        {
            AliasTypeCode alias = (AliasTypeCode)super.getContentTypeCode();
            return alias.getContentTypeCode();
        }
        else if(super.getContentTypeCode() instanceof ContainerTypeCode)
        {
            ContainerTypeCode container = (ContainerTypeCode)super.getContentTypeCode();
            if (container.getContentTypeCode() instanceof AliasTypeCode) {
                return ((AliasTypeCode)container.getContentTypeCode()).getContentTypeCode();
            }
        }

        return super.getContentTypeCode();
    }

    public String getOneLevelContentTypeCodeName()
    {
        if(super.getContentTypeCode() instanceof AliasTypeCode)
        {
            AliasTypeCode alias = (AliasTypeCode)super.getContentTypeCode();
            return alias.getName();
        }
        return super.getContentTypeCode().getIdlTypename();
    }

    public boolean isUnbound()
    {
        if(getTrueContentTypeCode() instanceof SequenceTypeCode)
        {
            return ((SequenceTypeCode)getTrueContentTypeCode()).isUnbound();
        }
        return false;
    }

    public ArrayList<String> getScopednameList()
    {
        if (m_scope_normal.isEmpty())
            return null;
        String[] namespaces = m_scope_normal.split("::");

        ArrayList<String> namespaceList = new ArrayList<>(Arrays.asList(namespaces));
        return namespaceList;
    }

    @Override
    public boolean isObjectType() { return true; }

    public TypeCode getTypedefContentTypeCode()
    {
        return super.getContentTypeCode();
    }

    public String getName()
    {
        return m_name;
    }

    public String getNameOrigin()
    {
        return m_name_origin;
    }

    @Override
    public String getScopedname()
    {
        if(m_scope.isEmpty())
            return m_name;

        return m_scope + "::" + m_name;
    }

    public String getCScopednamenormal()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal.replace("::", "_") + "_" + m_name;
    }

    public String getScopednamenormal()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal + "::" + m_name;
    }

    public String getROS2Scopedname()
    {
        if(m_scope.isEmpty())
            return m_name;

        return m_scope + "::dds_::" + m_name + "_";
    }

    public String getROS2Scopednamenormal()
    {
        if(m_scope_normal.isEmpty())
            return m_name;

        return m_scope_normal + "::dds_::" + m_name + "_";
    }

    public String getScope()
    {
        return m_scope;
    }

    public String getAliasScopeNormal()
    {
        return m_scope_normal;
    }

    public boolean getHasScope()
    {
        return !m_scope.isEmpty();
    }

    @Override
    public String getCppTypename()
    {
        StringTemplate st = getCppTypenameFromStringTemplate();
        st.setAttribute("name", getScopedname());
        return st.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate st = getCTypenameFromStringTemplate();
        st.setAttribute("name", getTrueContentTypeCode().getCTypename());
        return st.toString();
    }

    @Override
    public String getJavaTypename()
    {
        StringTemplate st = getJavaTypenameFromStringTemplate();
        st.setAttribute("name", getTypedefContentTypeCode().getJavaTypename());
        return st.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate st = getRustTypenameFromStringTemplate();
        st.setAttribute("name", getTypedefContentTypeCode().getRustTypename());
        return st.toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("name", getTrueContentTypeCode().getIdlTypename());
        return st.toString();
    }

    @Override
    public String getIdlFileTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("name", getName());
        return st.toString();
    }

    @Override
    public String getXmlTypename()
    {
        StringTemplate st = getXmlTypenameFromStringTemplate();
        st.setAttribute("name", getScopednamenormal());
        return st.toString();
    }

    @Override
    public String getStType()
    {
        return super.getContentTypeCode().getStType();
    }

    @Override
    public boolean isPrimitive()
    {
        return super.getContentTypeCode().isPrimitive();
    }

    @Override
    public boolean isIsStringType() { return super.getContentTypeCode().isIsStringType(); }

    @Override
    public boolean isIsWStringType() { return super.getContentTypeCode().isIsWStringType(); }

    @Override
    public String getInitialValue()
    {
        return super.getContentTypeCode().getInitialValue();
    }

    public Pair<Integer, Integer> getMaxSerializedSize(int currentSize, int lastDataAligned)
    {
        // TODO
        return null;
    }

    public int getMaxSerializedSizeWithoutAlignment(int currentSize)
    {
        // TODO
        return 0;
    }

    /*** Functions to know the type in string templates ***/
    @Override
    public boolean isIsSetType()
    {
        return super.getContentTypeCode().isIsSetType();
    }

    public boolean isObjectArray()
    {
        return super.getContentTypeCode().isObjectArray();
    }

    @Override
    public boolean isIsType_d()
    {
        return super.getContentTypeCode().isIsType_d();
    }

    @Override
    public boolean isIsArrayType()
    {
        return super.getContentTypeCode().isIsArrayType();
    }

    @Override
    public boolean isIsMapType()
    {
        return super.getContentTypeCode().isIsMapType();
    }

    @Override
    public boolean isStringType()
    {
        return super.getContentTypeCode().isStringType();
    }

    @Override
    public boolean isDefault() {
        return super.getContentTypeCode().isDefault();
    }

    @Override
    public boolean isIsType_a()
    {
        return super.getContentTypeCode().isIsType_a();
    }

    @Override
    public boolean isIsType_c()
    {
        return super.getContentTypeCode().isIsType_c();
    }

    @Override
    public boolean isIsType_f()
    {
        return super.getContentTypeCode().isIsType_f();
    }

    public boolean isIsType_e()
    {
        return super.getContentTypeCode().isIsType_e();
    }

    public boolean isIsType_5()
    {
        return super.getContentTypeCode().isIsType_5();
    }

    public boolean isIsType_6()
    {
        return super.getContentTypeCode().isIsType_6();
    }

    public String getContentTypeCodeJavaTypename()
    {
        if (getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getContentTypeCodeJavaTypename();
        }
        return null;
    }

    public List<String> getDimensionsTrueValue()
    {
        if (getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getDimensionsTrueValue();
        }
        return null;
    }

    @Override
    public boolean isIsSequenceType() { return super.getContentTypeCode().isIsSequenceType(); }

    public boolean isIsType_10()
    {
        return true;
    }

    @Override
    public boolean isIsType_uint8()
    {
        return super.getContentTypeCode().isIsType_uint8();
    }

    @Override
    public boolean isIsType_1e()
    {
        return super.getContentTypeCode().isIsType_1e();
    }

    @Override
    public String getTypeIdentifier()
    {
        return "0xF1U";
    }

    /*** End of functions to know the type in string templates ***/

    /*** Functions that alias has to export because some typecodes have them*/
    public List<Member> getAllLikeStructMembers() {
        return super.getContentTypeCode().getAllLikeStructMembers();
    }

    public String getMaxsize()
    {
        return super.getContentTypeCode().getMaxsize();
    }

    public boolean isMaxsizeGreaterThan1()
    {
        return super.getContentTypeCode().isMaxsizeGreaterThan1();
    }

    public boolean isMaxsizeGreaterThan0()
    {
        return super.getContentTypeCode().isMaxsizeGreaterThan0();
    }

    public String getJniScopedname()
    {
        return super.getContentTypeCode().getJniScopedname();
    }

    public String getJniScope()
    {
        return super.getContentTypeCode().getJniScope();
    }

    public boolean isSetPname(String pname)
    {
        return super.getContentTypeCode().isSetPname(pname);
    }

    public boolean isSetEmptyPname()
    {
        return super.getContentTypeCode().isSetEmptyPname();
    }

    public String getPname()
    {
        return super.getContentTypeCode().getPname();
    }

    public boolean setPname(String pname)
    {
        return super.getContentTypeCode().setPname(pname);
    }

    public boolean isSetPname()
    {
        return super.getContentTypeCode().isSetPname();
    }

    public List<Member> getMembers()
    {
        return super.getContentTypeCode().getMembers();
    }

    public String getEmptyArrayExtension()
    {
        return super.getContentTypeCode().getEmptyArrayExtension();
    }

    public String getLeftEmptyArrayExtension()
    {
        return super.getContentTypeCode().getLeftEmptyArrayExtension();
    }

    public String getRustLeftEmptyArrayExtension()
    {
        return super.getContentTypeCode().getRustLeftEmptyArrayExtension();
    }

    public String getLeftEmptyArrayExtensionByNum(int size)
    {
        return super.getContentTypeCode().getLeftEmptyArrayExtensionByNum(size);
    }

    public List<String> getForListSendBegin(String my_name)
    {
        return super.getContentTypeCode().getForListSendBegin(my_name);
    }

    public String getForListSendMid(String my_name)
    {
        return super.getContentTypeCode().getForListSendMid(my_name);
    }

    public List<String> getForListSendEnd(String my_name)
    {
        return super.getContentTypeCode().getForListSendEnd(my_name);
    }

    public List<String> getForListGetBegin(String my_name)
    {
        return super.getContentTypeCode().getForListGetBegin(my_name);
    }

    public String getForListGetMid(String my_name)
    {
        return super.getContentTypeCode().getForListGetMid(my_name);
    }

    public List<String> getForListGetEnd(String my_name)
    {
        return super.getContentTypeCode().getForListGetEnd(my_name);
    }

    public String getSize()
    {
        return super.getContentTypeCode().getSize();
    }

    public int getMaxsizeInt()
    {
        return super.getContentTypeCode().getMaxsizeInt();
    }

    public TypeCode getKeyTypeCode()
    {
        return super.getContentTypeCode().getKeyTypeCode();
    }

    public String getXMLMaxsize()
    {
        return super.getContentTypeCode().getXMLMaxsize();
    }

    public TypeCode getValueTypeCode()
    {
        return super.getContentTypeCode().getValueTypeCode();
    }

    public List<String> getDimensions()
    {
        if(getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getDimensions();
        }

        return null;
    }

    public String getCTypeDimensions()
    {
        if(getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getCTypeDimensions();
        }

        return "";
    }

    @Override
    public String getCwrapperTypename()
    {
        return super.getContentTypeCode().getCwrapperTypename();
    }

    public List<String> getDimensionsWithAlias()
    {
        if(getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getDimensionsWithAlias();
        }

        return null;
    }

    public String getDimensionsInXml()
    {
        if(getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getDimensionsInXml();
        }

        return null;
    }

    public boolean isHasKey() {
        if(getTrueContentTypeCode() instanceof StructTypeCode) {
            return ((StructTypeCode)getTrueContentTypeCode()).isHasKey();
        }

        return false;
    }
    public String getRustRightTypeDimensions()
    {
        if(getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getRustRightTypeDimensions();
        }

        return "";
    }
    public String getArrayExtension()
    {
        if(getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode)getTrueContentTypeCode()).getArrayExtension();
        }

        return "";
    }

    public String getScopenormal()
    {
        return super.getContentTypeCode().getScopenormal();
    }

    public boolean isInClass() {
        return super.getContentTypeCode().isInClass();
    }

    public int getEnumSize()
    {
        if(getTrueContentTypeCode() instanceof EnumTypeCode) {
            return ((EnumTypeCode)getTrueContentTypeCode()).getEnumSize();
        }

        return 1;
    }
//-------------maxSerializedSize----------------//
    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        return ((TypeCode) getTypedefContentTypeCode()).maxSerializedSize(current_alignment);
    }

    @Override
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64) throws RuntimeGenerationException
    {
        return ((TypeCode) getTypedefContentTypeCode()).maxPlainTypeSerializedSize(current_alignment, align64);
    }

    public boolean isNotZeroArray()
    {
        if (getTrueContentTypeCode() instanceof ArrayTypeCode) {
            return ((ArrayTypeCode) getTrueContentTypeCode()).isNotZeroArray();
        }
        return true;
    }
//-------------maxSerializedSize----------------//

    /*** End of functions that alias has to export because some typecodes have them*/

    private String m_name = null;

    private String m_scope = null;

    private String m_scope_normal = null;

    private String m_name_origin = null;
}
