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
// 2024-03 ~ 2025-05: idlgen optimization: xcdr support, typeobject improvement, @vbsconfig function, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.tree;

import java.util.Map;
import java.util.HashMap;
import java.util.Collection;
import java.util.List;
import java.util.ArrayList;

public class Annotation implements Cloneable {

    public static final String autoid_str = "autoid";
    public static final String autoid_enum_str = "AutoidKind";
    public static final String autoid_sequential_str = "SEQUENTIAL";
    public static final String autoid_sequential_value_str = "0";
    public static final String autoid_hash_str = "HASH";

    public static final String hashid_str = "hashid";
    public static final String id_str = "id";
    public static final String key_str = "key";

    public static final String extensibility_str = "extensibility";
    public static final String extensibility_enum_str = "ExtensibilityKind";
    public static final String final_str = "final";
    public static final String appendable_str = "appendable";
    public static final String mutable_str = "mutable";
    public static final String ex_final_str = "FINAL";
    public static final String ex_final_val = "0";
    public static final String ex_appendable_str = "APPENDABLE";
    public static final String ex_appendable_val = "1";
    public static final String ex_mutable_str = "MUTABLE";
    public static final String vbs_config_str = "vbsconfig";
    public static final String ex_mutable_val = "2";

    public static final String value_str = "value";

    public static final String method_str = "method";

    public Annotation(AnnotationDeclaration declaration)
    {
        m_declaration = declaration;
        m_members = new HashMap<String, AnnotationMember>();

        for(AnnotationMember ann : declaration.getMembers())
        {
            AnnotationMember member = new AnnotationMember(ann);
            m_members.put(member.getName(), member);
        }
    }

    public Annotation(Annotation other) {
        if (other.m_declaration != null) {
            this.m_declaration = new AnnotationDeclaration(other.m_declaration);
        } else {
            this.m_declaration = null;
        }

        this.m_members = new HashMap<String, AnnotationMember>();
        for (Map.Entry<String, AnnotationMember> entry : other.m_members.entrySet()) {
            this.m_members.put(entry.getKey(), new AnnotationMember(entry.getValue()));
        }
    }

    public String getName()
    {
        if(m_declaration != null)
        {
            return m_declaration.getName();
        }

        return null;
    }

    public boolean addValue(String value)
    {
        if(m_members.size() != 1)
            return false;

        ((AnnotationMember)m_members.values().toArray()[0]).setValue(value);

        return true;
    }

    public boolean addValue(String attribute, String value)
    {
        AnnotationMember member = m_members.get(attribute.toLowerCase());

        if(member != null)
        {
            member.setValue(value);
        }
        else
            return false;

        return true;
    }

    public boolean addValue(String attribute, String pre_value,
                    String suffix_value)
    {
        AnnotationMember member = m_members.get(attribute);

        if(member != null)
        {
            member.setValue(pre_value, suffix_value);
        }
        else
            return false;

        return true;
    }

    public String getValue()
    {
        if(m_members.size() != 1) return null;

        return ((AnnotationMember)m_members.values().toArray()[0]).getValue();
    }

    public String getValue(String attribute)
    {
        attribute = attribute.toLowerCase();
        return m_members.get(attribute) != null ? m_members.get(attribute).getValue() : null;
    }

    public Map<String, String> getSuffixValue(String attribute) {
        Map<String, String> resultMap = new HashMap<>();

        if (m_vbsconfig_members == null) {
            System.err.println(
                String.format("ERROR: vbsconfig_members is null, cannot retrieve suffix values for attribute '%s'",
                            attribute)
            );
            return resultMap;
        }

        List<AnnotationMember> membersList = m_vbsconfig_members.get(attribute);

        if (membersList == null || membersList.isEmpty()) {
            System.err.println(
                String.format("WARN: No AnnotationMember list found for attribute '%s', returning empty map",
                              attribute)
            );
            return resultMap;
        }

        for (AnnotationMember member : membersList) {
            resultMap.put(member.getValue(), member.getSufixValue());
        }

        return resultMap;
    }

    public Map<String, AnnotationMember> getValues()
    {
        return m_members;
    }

    public Collection<AnnotationMember> getValueList()
    {
        return m_members.values();
    }

    public void setConfigMembers(Map<String, AnnotationMember> newMembers) {
        if (m_vbsconfig_members == null || m_vbsconfig_members.isEmpty()) {
            m_vbsconfig_members = new HashMap<>();

            if (m_members != null) {
                for (Map.Entry<String, AnnotationMember> entry : m_members.entrySet()) {
                    String key = entry.getKey();
                    AnnotationMember member = entry.getValue();

                    List<AnnotationMember> membersList = new ArrayList<>();
                    membersList.add(member);

                    m_vbsconfig_members.put(key, membersList);
                }
            }
        }

        if (newMembers != null) {
            for (Map.Entry<String, AnnotationMember> entry : newMembers.entrySet()) {
                String key = entry.getKey();
                AnnotationMember member = entry.getValue();

                List<AnnotationMember> members =
                    m_vbsconfig_members.computeIfAbsent(key, k -> new ArrayList<>());
                members.add(member);
            }
        }
    }

    public boolean isVbsConfig() {
        return getName().equals(vbs_config_str);
    }

    @Override
    public Annotation clone() {
        return new Annotation(this);
    }

    private HashMap<String, AnnotationMember> m_members = null;
    private HashMap<String, List<AnnotationMember>> m_vbsconfig_members = null;
    private AnnotationDeclaration m_declaration = null;
}
