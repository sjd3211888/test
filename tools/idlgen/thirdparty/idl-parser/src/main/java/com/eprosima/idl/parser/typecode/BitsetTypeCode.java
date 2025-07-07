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
// 2023-05 ~ 2025-05: Enhancement of idlgen and idl2xml: supporting idl2xml parser, rpc build optimization, tostring/get_type_name interfaces, xcdr, random idl file generation tool, type validation, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.HashMap;

import org.antlr.stringtemplate.StringTemplate;

public class BitsetTypeCode extends MemberedTypeCode
{
    public BitsetTypeCode(String scope, String name, String namespace)
    {
        super(Kind.KIND_BITSET, scope, name, namespace);
        m_bitfields = new LinkedHashMap<String, Bitfield>();
        m_parents = new ArrayList<BitsetTypeCode>();
    }

    @Override
    public boolean isIsBitsetType(){return true;}

    @Override
    public boolean isObjectType() { return true; }

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

    public List<Bitfield> getBitfields()
    {
        return getBitfields(false);
    }

    public List<Bitfield> getBitfields(boolean includeParents)
    {
        ArrayList<Bitfield> result = new ArrayList<Bitfield>();

        if (includeParents)
        {
            for (BitsetTypeCode m_parent : m_parents)
            {
                result.addAll(m_parent.getBitfields());
            }
        }

        result.addAll(m_bitfields.values());
        return result;
    }

    public List<Bitfield> getAllBitfields() // Alias for getBitfields(true) for stg
    {
        return getBitfields(true);
    }

    public boolean addBitfield(Bitfield bitfield)
    {
        if(!m_bitfields.containsKey(bitfield.getName()))
        {
            m_bitfields.put(bitfield.getName(), bitfield);
            bitfield.setBasePosition(m_current_base);
            m_current_base += bitfield.getSpec().getBitSize();
            return true;
        }
        return false;
    }

    public void addParent(BitsetTypeCode parent)
    {
        m_parents.add(parent);
    }

    public List<BitsetTypeCode> getParents()
    {
        return m_parents;
    }

    public int getBitSize()
    {
        int size = 0;
        for (Bitfield bf : m_bitfields.values())
        {
            size += bf.getSpec().getBitSize();
        }
        return size;
    }

    public int getRustsize()
    {
        int size =getBitSize();
        return (size + 31) / 32;
    }

    @Override
    public boolean isIsPlain()
    {
        for (Member member : m_bitfields.values())
        {
            if (!member.isIsPlain())
            {
                return false;
            }
        }
        return true;
    }

    public int getFullBitSize()
    {
        int size = 0;

        if (m_parents != null)
        {
            for (BitsetTypeCode tmp_type : m_parents) {
                size += tmp_type.getFullBitSize();
            }
        }

        for (Bitfield bf : m_bitfields.values())
        {
            size += bf.getSpec().getBitSize();
        }
        return size;
    }

    public boolean isHasInheritance()
    {
        return m_parents.size() != 0;
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

        int full_bit_size = getFullBitSize();

        if (9 > full_bit_size)
        {
            current_alignment += 1;
        }
        else if (17 > full_bit_size)
        {
            current_alignment += 2 + TypeCode.cdr_alignment(current_alignment, 2);
        }
        else if (33 > full_bit_size)
        {
            current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);
        }
        else
        {
            current_alignment += 8 + TypeCode.cdr_alignment(current_alignment, align64);
        }

        return current_alignment - initial_alignment;
    }
//-------------maxSerializedSize----------------//

    @Override
    public String getHashstirng() {
        StringBuilder bitsetString = new StringBuilder();
        if (m_parents.size() != 0) {
            bitsetString.append("parent ");
            for (BitsetTypeCode parent : m_parents) {
                bitsetString.append(parent.getHash()).append(" ");
            }
        }

        StringBuilder memberString = new StringBuilder();
        for (Bitfield member : getBitfields()) {
            memberString.append("bitfield<")
                        .append(member.getSpec().getBitSize())
                        .append(", " + member.getSpec().getTypecode().getHashstirng())
                        .append("> ")
                        .append(member.getName() != null ? member.getName() + " " : "");
        }

        String result = bitsetString.toString() + memberString.toString();

        if (result.endsWith(" ")) {
            result = result.substring(0, result.length() - 1);
        }
        return result;
    }

    private ArrayList<BitsetTypeCode> m_parents = null;
    private LinkedHashMap<String, Bitfield> m_bitfields = null;
    private int m_current_base = 0;
}
