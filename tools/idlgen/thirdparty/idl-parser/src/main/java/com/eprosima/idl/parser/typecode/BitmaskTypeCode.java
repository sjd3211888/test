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
// 2023-05 ~ 2025-05: Enhancement of idlgen and idl2xml: supporting pubsub mode java/jni code generation, idl2xml parser, rpc build optimization, tostring/get_type_name interfaces, xcdr, random idl file generation tool, type validation, UpdateTest coverage optimization, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

import com.eprosima.idl.parser.tree.Annotation;
import com.eprosima.idl.context.Context;

import org.antlr.stringtemplate.StringTemplate;

public class BitmaskTypeCode extends MemberedTypeCode
{
    public BitmaskTypeCode(String scope, String name, String namespace)
    {
        super(Kind.KIND_BITMASK, scope, name, namespace);
        m_bitmasks = new LinkedHashMap<String, Bitmask>();
        m_value_bitmasks = new LinkedHashMap<Integer, Bitmask>();
    }

    public BitmaskTypeCode(String scope, String name, Integer bit_bound, String namespace)
    {
        super(Kind.KIND_BITMASK, scope, name, namespace);
        m_bit_bound = bit_bound;
        is_bound = true;
        m_bitmasks = new LinkedHashMap<String, Bitmask>();
        m_value_bitmasks = new LinkedHashMap<Integer, Bitmask>();
    }

    @Override
    public boolean isPrimitive() {return true;}

    @Override
    public boolean isIsBitmaskType(){return true;}

    @Override
    public boolean isObjectType() { return true; }

    @Override
    public String getSize()
    {
        if (m_bit_bound <= 8)
        {
            return "1";
        }
        else if (m_bit_bound <= 16)
        {
            return "2";
        }
        else if (m_bit_bound <= 32)
        {
            return "4";
        }
        else if (m_bit_bound <= 64)
        {
            return "8";
        }
        return "4";
    }

    @Override
    public String getInitialValue()
    {
        // if(getMembers().size() > 0)
        // {
        //    return (getScope() != null ? getScope() + "::" : "") + getMembers().get(0).getName();
        // }

        return ""; // Don't initialize bitmask
    }

    public String getRustInitialValue()
    {
        if(getMembers().size() > 0)
        {
           return getRustTypename() + "::" + getMembers().get(0).getName();
        }

        return ""; // Don't initialize bitmask
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
        st.setAttribute("name", getScopedname());
        return st.toString();
    }

    public List<Bitmask> getBitmasks()
    {
        return new ArrayList<Bitmask>(m_bitmasks.values());
    }

    public boolean addBitmask(Bitmask bitmask)
    {
        if (bitmask.getPosition() == -1)
        {
            if (bitmask.getAnnotations().get("position") != null)
            {
                // Position from attribute
                return addBitmask(bitmask, Integer.parseInt(bitmask.getAnnotations().get("position").getValue()));
            }
            // Position autoassigned
            return addBitmask(bitmask, m_current_base);
        }
        // Position by code.
        return addBitmask(bitmask, bitmask.getPosition());
    }

    private boolean addBitmask(Bitmask bitmask, int position)
    {
        if (position < 0 || position >= m_bit_bound) return false; // Out of bounds

        if (m_value_bitmasks.containsKey(position)) return false; // Position already taken

        if(m_bitmasks.containsKey(bitmask.getName())) return false; // Already exists

        bitmask.setPosition(position);
        m_value_bitmasks.put(position, bitmask);
        m_bitmasks.put(bitmask.getName(), bitmask);
        addMember(bitmask); // Use members too.
        m_current_base = position + 1;

        return true;
    }

    public void setParent(BitmaskTypeCode parent)
    {
        m_parent = parent;
    }

    public BitmaskTypeCode getParent()
    {
        return m_parent;
    }

    public int getBitBound()
    {
        return m_bit_bound;
    }

    public boolean isBounded()
    {
        return is_bound;
    }

    public String getBoundTail()
    {
        if (m_bit_bound <= 8)
        {
            return "";
        }
        else if (m_bit_bound <= 16)
        {
            return "";
        }
        else if (m_bit_bound <= 32)
        {
            return "u";
        }
        else if (m_bit_bound <= 64)
        {
            return "ull";
        }
        return "";
    }

    public String getBoundType()
    {
        if (m_bit_bound <= 8)
        {
            return " : uint8_t";
        }
        else if (m_bit_bound <= 16)
        {
            return " : uint16_t";
        }
        else if (m_bit_bound <= 32)
        {
            return " : uint32_t";
        }
        else if (m_bit_bound <= 64)
        {
            return " : uint64_t";
        }
        return "";
    }

    public String getCastingType()
    {
        if (m_bit_bound <= 8)
        {
            return "uint8_t";
        }
        else if (m_bit_bound <= 16)
        {
            return "uint16_t";
        }
        else if (m_bit_bound <= 32)
        {
            return "uint32_t";
        }
        else if (m_bit_bound <= 64)
        {
            return "uint64_t";
        }
        return "";
    }

    public String getRustCastingType() {
        if (m_bit_bound <= 8)
        {
            return "u8";
        }
        else if (m_bit_bound <= 16)
        {
            return "u16";
        }
        else if (m_bit_bound <= 32)
        {
            return "u32";
        }
        else if (m_bit_bound <= 64)
        {
            return "u64";
        }
        return "";
    }

    public String getJavaCastingType()
    {
        if (m_bit_bound <= 8)
        {
            return "short";
        }
        else if (m_bit_bound <= 16)
        {
            return "int";
        }
        else if (m_bit_bound <= 32)
        {
            return "long";
        }
        return "";
    }

    @Override
    public void addAnnotation(Context ctx, Annotation annotation)
    {
        super.addAnnotation(ctx, annotation);
        if (annotation.getName().equals("bit_bound"))
        {
            is_bound = true;
            m_bit_bound = Integer.parseInt(annotation.getValue());
        }
    }

    public int getEnumSize()
    {
        return getMembers().size() == 0 ? 1 : getMembers().size();
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
        long size = Long.parseLong(getSize(), 10);

        current_alignment += size + TypeCode.cdr_alignment(current_alignment, 4 < size ? align64 : size);

        return current_alignment - initial_alignment;
    }
//-------------maxSerializedSize----------------//

    @Override
    public String getHashstirng() {
        StringBuilder bitmaskString = new StringBuilder();
        bitmaskString.append(getScopednamenormal()).append(" ");
        bitmaskString.append("bit_bound ").append(m_bit_bound).append(" ");

        StringBuilder memberString = new StringBuilder();
        for (Member member : super.getMembers()) {
            memberString.append(member.getName())
                        .append(" ")
                        .append(((Bitmask) member).getPosition())
                        .append(" ");
        }

        if (memberString.length() > 0) {
            memberString.setLength(memberString.length() - 1);
        }

        return bitmaskString.toString() + memberString.toString();
    }

    private BitmaskTypeCode m_parent = null;
    private LinkedHashMap<String, Bitmask> m_bitmasks = null;
    private LinkedHashMap<Integer, Bitmask> m_value_bitmasks = null;
    private Integer m_bit_bound = 32;
    private int m_current_base = 0;
    private boolean is_bound = false;
}
