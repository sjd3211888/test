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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting idl2xml parser, RTI idl import and code generation, rpc build optimization, cpp generation fix, tostring/get_type_name/underlying interfaces, enum as length assignment, typename separator optimization, union fix, typedef/typeobject fix, xcdr, type validation, @autoid issue fix, production efficiency optimization, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import org.antlr.stringtemplate.StringTemplate;

public class EnumTypeCode extends MemberedTypeCode
{
    public EnumTypeCode(String scope, String name, String namespace)
    {
        super(Kind.KIND_ENUM, scope, name, namespace);
        default_member = null;
        enum_value = 0;
    }

    public EnumTypeCode(EnumTypeCode other)
    {
        super(other);
        this.default_member = other.getDefaultMember();
        this.enum_value = other.getEnumValue();
    }

    @Override
    public boolean isPrimitive() {return true;}

    @Override
    public boolean isIsType_c(){return true;}

    @Override
    public String getTypeIdentifier()
    {
        return "0xF1U";
    }

    @Override
    public boolean isObjectType() { return true; }

    public int getEnumValue()
    {
        return enum_value;
    }

    public void addMember(EnumMember member)
    {
        if (member == null)
            return;
        if (!member.getValue().equals("")) {
            enum_value = evaluateMathExpression(member.getValue()) + 1;
        } else {
            member.setValue(Integer.toString(enum_value++));
        }
        if (super.isEmpty()) {
            default_member = member;
        }
        addMember((Member)member);
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
        st.setAttribute("name", getCScopedname());
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
        st.setAttribute("name", getJavaScopedname());
        return st.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate st = getRustTypenameFromStringTemplate();
        st.setAttribute("name", getRustScopedname());
        return st.toString();
    }

    @Override
    public String getIdlTypename()
    {
        StringTemplate st = getIdlTypenameFromStringTemplate();
        st.setAttribute("name", getScopednamenormal());
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
    public String getInitialValue()
    {
        if(getMembers().size() > 0)
        {
            return (getScope() != null ? getScopedname() + "::" : getName()) + getMembers().get(0).getName();
        }

        return "";
    }

    @Override
    public String getRustInitialValue()
    {
        if(getMembers().size() > 0)
        {
            return getName()+"::"+ getMembers().get(0).getName();
        }

        return "";
    }

    @Override
    public String getJavaInitialValue()
    {
        if(getMembers().size() > 0)
        {
            return javapackage + getJavaScopedname() + "." + getMembers().get(0).getName();
        }

        return "";
    }

    public String getInitialMember()
    {
        if(getMembers().size() > 0)
        {
            return getMembers().get(0).getName();
        }

        return "";
    }

    public int getEnumSize()
    {
        return getMembers().size() == 0 ? 1 : getMembers().size();
    }

    public String getValueByName(String name)
    {
        for (Member member : getMembers())
        {
            if(member.getName().equals(name)) {
                return ((EnumMember)member).getValue();
            }
        }
        return "";
    }

    /*public Pair<Integer, Integer> getMaxSerializedSize(int currentSize, int lastDataAligned)
    {
        int size = getSize();

        if(size <= lastDataAligned)
        {
            return new Pair<Integer, Integer>(currentSize + size, size);
        }
        else
        {
            int align = (size - (currentSize % size)) & (size - 1);
            return new Pair<Integer, Integer>(currentSize + size + align, size);
        }
    }

    public int getMaxSerializedSizeWithoutAlignment(int currentSize)
    {
        return currentSize + getSize();
    }*/

    @Override
    public String getSize()
    {
        return "4";
    }

    public EnumMember getDefaultMember()
    {
        return default_member;
    }
//-------------maxSerializedSize----------------//
    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        return maxPlainTypeSerializedSize(current_alignment, 8);
    }

    @Override
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64)
    {
        long initial_alignment = current_alignment;

        current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);

        return current_alignment - initial_alignment;
    }
//-------------maxSerializedSize----------------//

    public String getDecimalNumber(String numbeString) {
        if(numbeString.matches("^0[xX][0-9A-Fa-f]+$")) {
            int intValue = Integer.parseInt(numbeString.substring(2), 16);
            return String.valueOf(intValue);
        }
        return numbeString;
    }

    @Override
    public String getHashstirng() {
        StringBuilder enumString = new StringBuilder();
        enumString.append(getScopednamenormal()).append(" ");

        StringBuilder memberString = new StringBuilder();
        for (Member member : super.getMembers()) {
            memberString.append(member.getName())
                        .append(" ")
                        .append(getDecimalNumber(((EnumMember) member).getValue()))
                        .append(" ");
        }

        if (memberString.length() > 0) {
            memberString.setLength(memberString.length() - 1);
        }

        return enumString.toString() + memberString.toString();
    }

    private EnumMember default_member;
    private int enum_value;
}
