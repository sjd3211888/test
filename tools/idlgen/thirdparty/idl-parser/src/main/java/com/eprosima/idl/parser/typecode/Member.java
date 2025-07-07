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
// 2023-05 ~ 2025-05: Continuous enhancement of idlgen and idl2xml: supporting pubsub mode java/jni code generation, idl2xml parser and @Key annotation, rpc build optimization, Java typedef, tostring/get_type_name interfaces, adaptation to seq/array with nested bounded strings, xcdr, is_plain interface restoration, typeobject improvement, super large heap management, type validation, @vbsconfig, vbs idl to rti idl conversion, -PackOne option, abi compatibility optimization, toidlstring default value fix, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import com.eprosima.idl.parser.tree.Annotation;
import com.eprosima.idl.parser.tree.Notebook;
import com.eprosima.idl.context.Context;
import com.eprosima.idl.parser.exception.RuntimeGenerationException;

import java.util.Map;
import java.util.HashMap;
import java.util.Collection;
import java.util.List;
import java.util.Objects;

public class Member implements Notebook
{
    public Member()
    {
        m_annotations = new HashMap<String, Annotation>();
    }

    public Member(TypeCode typecode, String name)
    {
        m_typecode = typecode;
        m_name = name;
        m_annotations = new HashMap<String, Annotation>();
    }

    public Member(Member other)
    {
        this.m_typecode = other.m_typecode;
        this.m_name = other.m_name;
        this.m_annotations = new HashMap<String, Annotation>();
        for (Map.Entry<String, Annotation> entry : other.m_annotations.entrySet()) {
            this.m_annotations.put(entry.getKey(), new Annotation(entry.getValue()));
        }
    }

    public Member copy() {
        return new Member(this);
    }

    public String getName()
    {
        return m_name;
    }

    public boolean isHasName() {
        return !m_name.equals("");
    }

    public String getScopedname()
    {
        return m_name;
    }

    public String getJavaName() {
        if (m_name != null) {
            Character firstChar =Character.toUpperCase(m_name.charAt(0));
            String javaName = firstChar.toString();
            if (m_name.length() > 1) {
                javaName += m_name.substring(1);
            }
            return javaName;
        }
        return null;
    }

    /*
     * @brief This function is used with (previous c) types because array names contains [].
     */

    public TypeCode getTypecode()
    {
        return m_typecode;
    }

    public String getPname()
    {
        return m_pname;
    }

    public boolean isSetPname()
    {
        return m_typecode.isSetPname(m_name);
    }

    public boolean isSetEmptyPname()
    {
        return m_typecode.isSetEmptyPname();
    }

    public boolean setPname(String pname)
    {
        m_pname = pname;
        m_typecode.setPname(pname);
        return true;
    }

    public void setName(String name)
    {
        m_name = name;
    }

    public void setFlagName(String name)
    {
        m_flagname = name;
    }

    public String getFlagName()
    {
        return m_flagname;
    }

    public void setTypecode(TypeCode typecode)
    {
         m_typecode = typecode;
    }

    @Override
    public void addAnnotation(Context ctx, Annotation annotation)
    {
        if(annotation != null)
        {
            if (((annotation.getName().equals("key") || annotation.getName().equals("Key") )
                && isAnnotationNonSerialized())
                || (annotation.getName().equals("non_serialized") && isAnnotationKey()))
            {
                System.err.println("ERROR: Member " + m_name + " has incompatible annotations key and non_serialized.");
                System.exit(1);
            }
            else
            {
                m_annotations.put(annotation.getName(), annotation);
            }
        }
    }

    @Override
    public Map<String, Annotation> getAnnotations()
    {
        return m_annotations;
    }

    public Collection<Annotation> getAnnotationList()
    {
        return m_annotations.values();
    }

    public boolean isAnnotationOptional()
    {
        if (m_typecode == null || (m_typecode != null && !m_typecode.ctx.isHasMutable()))
            return false;
        Annotation ann = m_annotations.get("optional");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isAnnotationCompatible()
    {
        Annotation ann = m_annotations.get("compatible");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isAnnotationMustUnderstand()
    {
        Annotation ann = m_annotations.get("must_understand");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isAnnotationNonSerialized()
    {
        Annotation ann = m_annotations.get("non_serialized");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isAnnotationKey()
    {
        Annotation ann = m_annotations.get("key");
        if (ann == null)
        {
            ann = m_annotations.get("Key"); // Try old way
        }
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public Short getAnnotationBitBound()
    {
        Annotation ann = m_annotations.get("bit_bound");
        if (ann != null)
        {
            String value = ann.getValue();
            if (value.equals("-1"))
            {
                return null;
            }
            return Short.parseShort(value);
        }
        return null;
    }

    public boolean isAnnotationDefaultLiteral()
    {
        return m_annotations.get("default_literal") != null;
    }

    public String getAnnotationValue()
    {
        Annotation ann = m_annotations.get("value");
        if (ann != null)
        {
            return ann.getValue();
        }
        return null;
    }

    public boolean isKeyTrue()
    {
        Annotation ann = m_annotations.get("Key");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        ann = m_annotations.get("key");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public void setIsStringed()
    {
        m_is_stringed = true;
    }

    public boolean getIsStringed()
    {
        return m_is_stringed;
    }

    public Short getAnnotationPosition()
    {
        Annotation ann = m_annotations.get("position");
        if (ann != null)
        {
            String value = ann.getValue();
            if (value.equals("-1"))
            {
                return null;
            }
            return Short.parseShort(value);
        }
        return null;
    }

    public boolean isAnnotationDefault()
    {
        return m_annotations.get("default") != null;
    }

    public String getAnnotationDefaultValue()
    {
        Annotation ann = m_annotations.get("default");
        if (ann != null)
        {
            return ann.getValue();
        }
        return "";
    }

    public String getAnnotationDefaultValueForIdl()
    {
        Annotation ann = m_annotations.get("default");
        if (ann != null)
        {
            String value = ann.getValue();
            if (value != null)
            {
                return value.replace("\"", "\\\"");
            }
        }
        return "";
    }

    public String getAnnotationDefaultValueForXML()
    {
        Annotation ann = m_annotations.get("default");
        if (ann != null)
        {
            String value = ann.getValue();
            if (value != null)
            {
                return value.replace("\"", "");
            }
        }
        return "";
    }

    public boolean isIsPlain()
    {
        if (m_typecode != null && !isAnnotationOptional())
        {
            return m_typecode.isIsPlain();
        }
        return false;
    }

    public boolean isIsBounded()
    {
        if (m_typecode != null)
        {
            return m_typecode.isIsBounded();
        }
        return false;
    }

    public List<String> getForListSendBegin()
    {
        return m_typecode.getForListSendBegin(m_name);
    }

    public String getForListSendMid()
    {
        return m_typecode.getForListSendMid(m_name);
    }

    public List<String> getForListSendEnd()
    {
        return m_typecode.getForListSendEnd(m_name);
    }

    public List<String> getForListGetBegin()
    {
        return m_typecode.getForListGetBegin(m_name);
    }

    public String getForListGetMid()
    {
        return m_typecode.getForListGetMid(m_name);
    }

    public List<String> getForListGetEnd()
    {
        return m_typecode.getForListGetEnd(m_name);
    }

    public boolean isScoped()
    {
        return false;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || getClass() != obj.getClass()) {
            return false;
        }
        Member other = (Member) obj;
        return Objects.equals(this.getTypecode().getIdlTypename(), other.getTypecode().getIdlTypename());
    }

    @Override
    public int hashCode() {
        return Objects.hash(this.getTypecode().getIdlTypename());
    }

    public boolean isXMLTypedeftype() {
        return m_is_xml_typedef;
    }

    public void setXMLTypedeftype() {
        m_is_xml_typedef = true;
    }

    public void set_index(int index)
    {
        index_ = index;
    }

    public int getIndex()
    {
        return index_;
    }

    public boolean isAnnotationId()
    {
        return null != m_annotations.get(Annotation.id_str);
    }

    public String getAnnotationIdValue() throws RuntimeGenerationException
    {
        Annotation ann = m_annotations.get(Annotation.id_str);
        if (ann == null)
        {
            throw new RuntimeGenerationException("Error in member " + m_name + ": @" + Annotation.id_str +
                    " annotation not found.");
        }

        return ann.getValue();
    }

    public void set_id(int id)
    {
        id_ = id;
    }

    public int get_id()
    {
        return id_;
    }

    public boolean isIsHasInitId()
    {
        return id_ != MEMBER_ID_INVALID;
    }

    public boolean isAnnotationHashid()
    {
        return null != m_annotations.get(Annotation.hashid_str);
    }

    public String getAnnotationHashidValue() throws RuntimeGenerationException
    {
        Annotation ann = m_annotations.get(Annotation.hashid_str);
        if (ann == null)
        {
            throw new RuntimeGenerationException("Error in member " + m_name + ": @" + Annotation.hashid_str +
                    " annotation not found.");
        }

        return ann.getValue();
    }

    public String getId()
    {
        return String.format("0x%08x", id_);
    }

    public boolean isAnnotationExternal()
    {
        Annotation ann = m_annotations.get("external");
        if (ann != null)
        {
            return ann.getValue().toUpperCase().equals("TRUE");
        }
        return false;
    }

    public boolean isNeedsPointer()
    {
        return m_typecode.isForwarded() || (m_typecode.isNeedsPointer() && !isAnnotationOptional());
    }

    private static void indent(StringBuilder code, int level) {
        for (int i = 0; i < level; i++) {
            code.append("    ");
        }
    }

    private void generateLoop(StringBuilder code, int totalDimensions, int currentDimension,
    String arrayName, String offsetPtrName, String indexName) {

        indent(code, currentDimension);
        String varName = "var" + currentDimension;

        if (currentDimension == 0) {
            code.append("for (auto& ").append(varName).append(" : ").append(offsetPtrName).append(arrayName).append(") {\n");
        } else {
            String prevVarName = "var" + (currentDimension - 1);
            code.append("for (auto& ").append(varName).append(" : ").append(prevVarName).append(") {\n");
        }

        if (currentDimension + 1 < totalDimensions) {
            generateLoop(code, totalDimensions, currentDimension + 1, arrayName, offsetPtrName, indexName);
        } else {
            indent(code, currentDimension + 1);
            code.append(varName).append(".updateTest(").append(indexName).append(");\n");
        }

        indent(code, currentDimension);
        code.append("}\n");
    }

    public String getUpdateTestString() {
        TypeCode tmp_typecode = m_typecode;
        if(tmp_typecode instanceof AliasTypeCode) {
            tmp_typecode = ((AliasTypeCode)tmp_typecode).getTrueContentTypeCode();
        }
        if(tmp_typecode.isIsArrayType()) {
            StringBuilder code = new StringBuilder();
            int totalDimensions = ((ArrayTypeCode)tmp_typecode).getDimensionsSize();
            String offsetPtrName = tmp_typecode.ctx.isHideInternals() ? "offset_ptr->" : "", indexName = "index";
            if (tmp_typecode.isNeedsPointer() || isAnnotationOptional()) {
                offsetPtrName = "*" + offsetPtrName;
            }
            generateLoop(code, totalDimensions, 0, "m_" + m_name, offsetPtrName, indexName);
            return code.toString();
        }
        return "";
    }

    public String getHashstirng() {
        TypeCode tmp_typecode = m_typecode;
        if(tmp_typecode instanceof AliasTypeCode) {
            tmp_typecode = ((AliasTypeCode)tmp_typecode).getTrueContentTypeCode();
        }
        String inner_hash_string = "";
        if (tmp_typecode.isAnnotationMutable()) {
            inner_hash_string = "0";
        }
        if((tmp_typecode.isPrimitive() && !tmp_typecode.isIsType_c() && !tmp_typecode.isIsBitmaskType()) || tmp_typecode.isIsType_d() ||
            tmp_typecode.isIsType_f() || tmp_typecode.isIsSequenceType() || tmp_typecode.isIsMapType()) {
            //基础类型用名称
            inner_hash_string = tmp_typecode.getHashstirng();
        } else {
            //复杂类型用hash
            inner_hash_string = tmp_typecode.getHash();
        }
        return inner_hash_string + (m_name != null ? (" " + m_name) : "");
    }

    public boolean isPackOneAndPlain() {
        return m_typecode.ctx.isUsePackOne() && m_typecode.isIsPlain();
    }

    private String m_name = null;

    private String m_flagname = null;

    private String m_pname = null;

    private TypeCode m_typecode = null;

    private HashMap<String, Annotation> m_annotations = null;

    private boolean m_is_stringed = false;

    private boolean m_is_xml_typedef = false;
    private int index_ = 0;

    public static final int MEMBER_ID_INVALID = 0x0FFFFFFF;

    private int id_ = MEMBER_ID_INVALID;
}
