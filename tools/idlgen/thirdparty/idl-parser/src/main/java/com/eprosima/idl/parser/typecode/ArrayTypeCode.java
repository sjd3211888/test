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
// 2023-05 ~ 2025-05: Continuous optimization of idlgen and idl2xml: support for pubsub mode java/jni code generation, idl2xml parser, dynamic array type fix, RTI idl import and code generation, java typedef, tostring/get_type_name, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.typecode;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.regex.Pattern;

import com.eprosima.idl.parser.exception.RuntimeGenerationException;
import org.antlr.stringtemplate.StringTemplate;
import org.antlr.stringtemplate.StringTemplateGroup;

public class ArrayTypeCode extends ContainerTypeCode
{
    public ArrayTypeCode()
    {
        super(Kind.KIND_ARRAY);
        m_cont_map = new HashMap<>();
        m_dimensions = new ArrayList<String>();
        m_pname = "";
        useBBMode_ = false;
    }

    @Override
    public boolean isIsType_f(){return true;}

    @Override
    public String getTypeIdentifier()
    {
        return "TI_PLAIN_ARRAY_SMALL";
    }

    @Override
    public boolean isPlainType() { return true; }

    @Override
    public boolean isIsArrayType() { return true; }

    @Override
    public String getCppTypename()
    {
        StringTemplate first = null, second = null, fin = null;
        String prevf = null, prevs = null;

        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            first = cpptypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_first");
            second = cpptypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_second");
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                second.setAttribute("size", m_cont_map.get(m_dimensions.get(count)));
            } else {
                second.setAttribute("size", m_dimensions.get(count));
            }

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

        fin = getCppTypenameFromStringTemplate();
        fin.setAttribute("firs", prevf);
        fin.setAttribute("secon", prevs);
        fin.setAttribute("type", getContentTypeCode().getCppTypename());

        return fin.toString();
    }

    @Override
    public String getCTypename()
    {
        StringTemplate first = null, second = null, fin = null;
        String prevf = null, prevs = null;

        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            first = ctypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_first");
            second = ctypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_second");
            second.setAttribute("size", m_dimensions.get(count));

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

        fin = getCTypenameFromStringTemplate();
        fin.setAttribute("firs", prevf);
        fin.setAttribute("secon", prevs);
        fin.setAttribute("type", getContentTypeCode().getCTypename());
        return fin.toString();
    }

    @Override
    public String getCwrapperTypename()
    {
        StringTemplate first = null, second = null, fin = null;
        String prevf = null, prevs = null;

        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            first = ctypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_first");
            second = ctypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_second");
            second.setAttribute("size", m_dimensions.get(count));

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

        fin = getCTypenameFromStringTemplate();
        fin.setAttribute("firs", prevf);
        fin.setAttribute("secon", prevs);
        fin.setAttribute("type", getContentTypeCode().getCwrapperTypename());
        return fin.toString();
    }
    public String getCTypeDimensions()
    {
        String dimensions = getArrayExtension();

        // if(getContentTypeCode() instanceof StringTypeCode)
        // {
        //     dimensions += "[" + ((StringTypeCode)getContentTypeCode()).getMaxsize() + "]";
        // }

        return dimensions;
    }
    public String getRustRightTypeDimensions() {
        StringBuilder ret = new StringBuilder();

        for (int count = m_dimensions.size() - 1; count >= 0; --count) {
            if (m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                ret.append(m_cont_map.get(m_dimensions.get(count))).append("];");
            } else {
                ret.append(m_dimensions.get(count)).append("];");
            }
        }

        if (getContentTypeCode().isIsType_10()) {
            ret.append(((AliasTypeCode)getContentTypeCode()).getRustRightTypeDimensions());
        }
        if (ret.length() > 0 && ret.charAt(ret.length() - 1) == ';') {
            ret.setLength(ret.length() - 1);
        }
        return ret.toString();
    }

    // TODO Used in stringtemplate for RTI DDS types.
    public String getArrayExtension()
    {
        String ret = "";

        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                ret += "[" + m_cont_map.get(m_dimensions.get(count)) + "]";
            } else {
                ret += "[" + m_dimensions.get(count) + "]";
            }
        }
        if (getContentTypeCode().isIsType_10()) {
            ret += ((AliasTypeCode)getContentTypeCode()).getArrayExtension();
        }
        return ret;
    }

    @Override
    public String getJavaTypename()
    {
        StringTemplate fin = null;
        String prevf = null, prevs = null, alls = "";

        alls = getArrayExtension();
        fin = getJavaTypenameFromStringTemplate();
        fin.setAttribute("firs", "");
        fin.setAttribute("secon", alls);
        fin.setAttribute("type", getInnerContentTypeCode().getJavaTypename());
        return fin.toString();
    }

    @Override
    public String getRustTypename()
    {
        StringTemplate first = null, second = null, fin = null;
        String prevf = null, prevs = null;

        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            first = ctypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_first");
            second = ctypesgr.getInstanceOf("type_" + Integer.toHexString(Kind.KIND_ARRAY) + "_second");
            second.setAttribute("size", m_dimensions.get(count));

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

        fin = getRustTypenameFromStringTemplate();
        fin.setAttribute("firs", prevf);
        fin.setAttribute("secon", prevs);
        fin.setAttribute("type", getContentTypeCode().getRustTypename());
        return fin.toString();
    }

    public String getContentTypeCodeJavaTypename()
    {
        if (getContentTypeCode() instanceof AliasTypeCode) {
            return ((AliasTypeCode)getContentTypeCode()).getContentTypeCodeJavaTypename();
        }
        return getContentTypeCode().getJavaTypename();
    }

    public String getContentTypeCodeRustTypename()
    {
        return getContentTypeCode().getRustTypename();
    }

    public String getIdlTypename()
    {
        StringTemplate fin = null;
        String prevf = null, prevs = null, alls = "";

        alls = getArrayExtension();

        fin = getIdlTypenameFromStringTemplate();
        fin.setAttribute("firs", "");
        fin.setAttribute("secon", alls);
        fin.setAttribute("type", getContentTypeCode().getIdlTypename());
        return fin.toString();
    }

    @Override
    public boolean setPname(String pname)
    {
        m_pname = (pname == null ? "" : pname);
        return true;
    }

    @Override
    public String getPname()
    {
        return m_pname;
    }

    public String getXmlTypename()
    {
        return "Array_" + getContentTypeCode().getXmlTypename();
    }

    public void addDimension(String dimension)
    {
        m_dimensions.add(dimension);
    }

    public void addConstMap(String const_name, int const_size)
    {
        m_cont_map.put(const_name, const_size);
    }

    public List<String> getDimensions()
    {
        return m_dimensions;
    }

    public List<String> getTrueDimensions()
    {
        List<String> ret = new ArrayList<>();
        for (int count = 0; count < m_dimensions.size(); ++count)
        {
            String dimension = m_dimensions.get(count);
            if (dimension != null && !Pattern.matches("\\d+", dimension)) {
                Integer value = m_cont_map.get(dimension);
                ret.add(value != null ? value.toString() : "");
            } else {
                ret.add(dimension != null ? dimension : "");
            }
        }
        return ret;
    }

    public List<String> getDimensionsTrueValue()
    {
        List<String> true_dimensions = new ArrayList<String>();
        List<String> temp_dimensions = getDimensionsWithAlias();
        for(int count = 0; count < temp_dimensions.size(); ++count)
        {
            if(temp_dimensions.get(count) != null && !Pattern.matches("\\d+", temp_dimensions.get(count))) {
                true_dimensions.add(String.valueOf(m_cont_map.get(temp_dimensions.get(count))));
            } else {
                true_dimensions.add(temp_dimensions.get(count));
            }
        }
        return true_dimensions;
    }

    public List<String> getDimensionsWithAlias()
    {
        List<String> temp_dimensions = new ArrayList<>(m_dimensions);
        if (getContentTypeCode().isIsType_10()) {
            List<String> dimensionsWithAlias = ((AliasTypeCode) getContentTypeCode()).getDimensionsWithAlias();
            if (dimensionsWithAlias != null) {
                temp_dimensions.addAll(dimensionsWithAlias);
            }
        }
        return temp_dimensions;
    }

    public String getXMLMaxsize() {
        List<String> true_dimensions = getDimensionsTrueValue();
        return String.join("_", true_dimensions);
    }

    public String getSize()
    {
        String ret = "";

        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(ret.isEmpty())
                ret += "(";
            else
                ret += " * ";

            ret += m_dimensions.get(count);
        }

        if(!ret.isEmpty())
            ret += ")";

        return ret;
    }

    public boolean isMaxsizeGreaterThan1() {
        if (m_dimensions.isEmpty())
            return false;
        if (!m_cont_map.isEmpty()) {
            for (String key : m_cont_map.keySet()) {
                    Integer value = m_cont_map.get(key);
                    return value > 1;
                }
        }
        else
        {
            for (String dim : m_dimensions)
            {
                if (Integer.parseInt(dim) > 1) {
                    return true;
                }
            }
        }
        return false;
    }

    public boolean isMaxsizeGreaterThan0()
    {
        if (m_dimensions.isEmpty())
            return false;
        return true;
    }

    @Override
    public String getEstimateSize()
    {
        int tmpSize = 1;
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                tmpSize *= m_cont_map.get(m_dimensions.get(count));
            } else {
                tmpSize *= evaluateMathExpression(m_dimensions.get(count));
            }
        }
        return String.valueOf(tmpSize * evaluateMathExpression(getContentTypeCode().getEstimateSize() == null ? "0" : getContentTypeCode().getEstimateSize()));
    }

    public String getEmptyArrayExtension()
    {
        String ret = "";

        ret += getNoAliasContentTypeCode().getEmptyArrayExtension();
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            ret += "[]";
        }

        return ret;
    }

    public String getLeftEmptyArrayExtension()
    {
        String ret = "";
        ret += getNoAliasContentTypeCode().getLeftEmptyArrayExtension();
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            ret += "[";
        }

        return ret;
    }

    public String getRustLeftEmptyArrayExtension()
    {
        String ret = "";

        for(int count = 0; count < getDimensionsWithAlias().size(); ++count)
        {
            ret += "[";
        }

        return ret;
    }

    public String getLeftEmptyArrayExtensionByNum(int size)
    {
        String ret = "";

        for(int count = 0; count < size; ++count)
        {
            ret += "[";
        }

        return ret;
    }

    public List<String> getForListSendBegin(String my_name)
    {
        List<String> allMembers = new ArrayList<String>();
        char c = 'a';
        String ret = "";
        List<String> temp_dimensions = getDimensionsWithAlias();
        for(int count = 0; count < temp_dimensions.size(); ++count)
        {
            if (temp_dimensions.size() == 1) {
                if (getInnerContentTypeCode().isIsStringType()) {
                    ret +="jobjectArray tmp_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +" = reinterpret_cast<jobjectArray>(aArray_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +");\n";
                } else {
                    ret += getInnerContentTypeCode().getJNITypename()+"Array tmp_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +" = reinterpret_cast<"+ getInnerContentTypeCode().getJNITypename()+"Array>(aArray_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +");\n";
                    ret += getInnerContentTypeCode().getJNITypename()+"* aData_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +" = env->Get"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"ArrayElements(tmp_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +", nullptr);\n";
                }
            }
            ret += "for(jsize "+(char)(((int)c) + 1) +" = 0; "+(char)(((int)c) + 1)+" < "+temp_dimensions.get(count)+ "; "+ (char)(((int)c) + 1)+"++) {\n";
            if (count < temp_dimensions.size() - 2 ) {
                ret += "jobjectArray "+(char)(((int)c) + 1)+"Array_"+my_name + (getPname().equals("") ? "" : "_" + getPname());
                ret += " = reinterpret_cast<jobjectArray>(env->GetObjectArrayElement(reinterpret_cast<jobjectArray>("+c+"Array_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +"), "+(char)(((int)c) + 1)+"));\n";
            }
            if (count == temp_dimensions.size() - 2 ) {
                if (!(getInnerContentTypeCode().isIsType_d())) {
                    ret += getInnerContentTypeCode().getJNITypename()+"Array "+ (char)(((int)c) + 1) +"Array_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) + " = reinterpret_cast<"+getInnerContentTypeCode().getJNITypename()+"Array>(env->GetObjectArrayElement(reinterpret_cast<jobjectArray>("+c+"Array_"+my_name + (getPname().equals("") ? "" : "_" + getPname())+"), "+(char)(((int)c) + 1)+"));\n" ;
                    ret += getInnerContentTypeCode().getJNITypename()+"* "+(char)(((int)c) + 1)+"Data_"+my_name + (getPname().equals("") ? "" : "_" + getPname());
                    ret += " = env->Get"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"ArrayElements("+(char)(((int)c) + 1)+"Array_" + my_name + (getPname().equals("") ? "" : "_" + getPname()) + ", nullptr);\n";
                } else {
                    ret += "jobjectArray "+ (char)(((int)c) + 1) +"Array_" + my_name + (getPname().equals("") ? "" : "_" + getPname()) + " = reinterpret_cast<jobjectArray>(env->GetObjectArrayElement(reinterpret_cast<jobjectArray>("+c+"Array_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +"), "+(char)(((int)c) + 1)+"));\n" ;
                }
            }
            if (count == temp_dimensions.size() - 1 ) {
                if (getInnerContentTypeCode().isIsType_d()) {
                    if (temp_dimensions.size() == 1) {
                        ret += "jstring jstr = reinterpret_cast<jstring>(env->GetObjectArrayElement(tmp_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +", "+(char)(((int)c) + 1)+"));";
                    } else {
                        ret += "jstring jstr = reinterpret_cast<jstring>(env->GetObjectArrayElement(" + c + "Array_" + my_name + (getPname().equals("") ? "" : "_" + getPname())  + ", "+(char)(((int)c) + 1)+"));";
                    }
                } else {
                    ret += getInnerContentTypeCode().getJNITypename()+" value";
                    ret += " = "+c+"Data_"+my_name + (getPname().equals("") ? "" : "_" + getPname()) +"["+(char)(((int)c) + 1)+"];";
                }
            }

            allMembers.add(ret);
            c = (char)(((int)c) + 1);
            ret = "";
        }
        m_c = c;
        return allMembers;
    }

    public String getForListSendMid(String my_name)
    {
        char c = 'a';
        String ret = "";
        List<String> temp_dimensions = getDimensionsWithAlias();
        if (getInnerContentTypeCode().isIsType_d()) {
            ret += "if ( jstr != nullptr) {\n";
            ret += "    const char* str = env->GetStringUTFChars(jstr, nullptr);\n";
            String con = "";
            for(int count = 0; count < temp_dimensions.size(); ++count)
            {
                con += "["+(char)(((int)c) + 1)+"]";
                c = (char)(((int)c) + 1);
            }
            ret += "    cppArray" + my_name + (getPname().equals("") ? "" : "_" + getPname()) + con + " = std::string(str);\n";
            ret += "    env->ReleaseStringUTFChars(jstr, str);\n";
            ret += "} else {\n";
            ret += "    cppArray" + my_name + (getPname().equals("") ? "" : "_" + getPname()) + con + " = \"\";\n";
            ret += "}\n";
            ret += "env->DeleteLocalRef(jstr);";
        } else {
            ret += "cppArray" + my_name + (getPname().equals("") ? "" : "_" + getPname());
            for(int count = 0; count < temp_dimensions.size(); ++count)
            {
                ret += "["+(char)(((int)c) + 1)+"]";
                c = (char)(((int)c) + 1);
            }
            ret += " = value;";
        }
        return ret;
    }

    public List<String> getForListSendEnd(String my_name)
    {
        List<String> allMembers = new ArrayList<String>();
        char c = (char)(((int)m_c) - 1);
        String ret = "";
        List<String> temp_dimensions = getDimensionsWithAlias();
        for(int count = 0; count < temp_dimensions.size(); ++count)
        {
            ret += "}\n";
            if (count == 0) {
                if (!getInnerContentTypeCode().isIsType_d()) {
                    if (temp_dimensions.size() == 1) {
                        ret += "env->Release"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"ArrayElements(tmp_"+my_name + (getPname().equals("") ? "" : "_" + getPname())+", "+c+"Data_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", JNI_ABORT);";
                    } else {
                        ret += "env->Release"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"ArrayElements("+c+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", "+c+"Data_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", JNI_ABORT);";
                    }
                }
            }
            if ((int)c >= 'b')
                ret += "env->DeleteLocalRef("+c+"Array_"+my_name + (getPname().equals("") ? "" : "_" + getPname())+");";
            allMembers.add(ret);
            c = (char)(((int)c) - 1);
            ret = "";
        }

        return allMembers;
    }

    public List<String> getForListGetBegin(String my_name)
    {
        List<String> allMembers = new ArrayList<String>();
        char c = 'a';
        String ret = "";
        List<String> temp_dimensions = getDimensionsWithAlias();
        for(int count = temp_dimensions.size(); count > 0; --count)
        {
            if (count >= 2 ) {
                ret += "jobjectArray "+(char)(((int)c) + 1)+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname());
                ret += " = env->NewObjectArray("+temp_dimensions.get(temp_dimensions.size() - count)+", env->FindClass(\""+ getLeftEmptyArrayExtensionByNum(count - 1)+""+getInnerContentTypeCode().getJNISign()+"\"),nullptr);\n";
            }
            if (count == 1 ) {
                if(getInnerContentTypeCode().isIsType_d()) {
                    ret += "jobjectArray "+(char)(((int)c) + 1)+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname());
                    ret += " = env->NewObjectArray("+temp_dimensions.get(temp_dimensions.size() - count)+", env->FindClass(\""+ getLeftEmptyArrayExtensionByNum(count - 1)+"java/lang/String\"),nullptr);\n";
                } else {
                    ret += getInnerContentTypeCode().getJNITypename()+"Array "+(char)(((int)c) + 1)+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname());
                    ret += " = env->New"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"Array("+temp_dimensions.get(temp_dimensions.size() - count)+");\n";

                    ret += getInnerContentTypeCode().getJNITypename()+"* " + my_name+ (getPname().equals("") ? "" : "_" + getPname()) + "_data";
                    ret += " = env->Get"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"ArrayElements("+(char)(((int)c) + 1)+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", nullptr);\n";
                }
            }
            ret += "for(jsize "+(char)(((int)c) + 1) +" = 0; "+(char)(((int)c) + 1)+" < "+temp_dimensions.get(temp_dimensions.size() - count)+ "; "+ (char)(((int)c) + 1)+"++) {\n";
            allMembers.add(ret);
            c = (char)(((int)c) + 1);
            ret = "";
        }
        m_c = c;
        return allMembers;
    }

    public String getForListGetMid(String my_name)
    {
        char c = 'a';
        String ret = "";
        List<String> temp_dimensions = getDimensionsWithAlias();
        if (getInnerContentTypeCode().isIsType_d()) {
            ret += "if (cppArray" + my_name + (getPname().equals("") ? "" : "_" + getPname());
            String con = "";
            for(int count = 0; count < temp_dimensions.size(); ++count)
            {
                con += "["+(char)(((int)c) + 1)+"]";
                c = (char)(((int)c) + 1);
            }
            ret += con;
            ret += " == \"\") {\n";
            ret += " env->SetObjectArrayElement("+c+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", "+c+", nullptr);\n";
            ret += "} else {\n";
            ret += " env->SetObjectArrayElement("+c+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", "+c+", env->NewStringUTF(cppArray" + my_name + (getPname().equals("") ? "" : "_" + getPname()) + con + ".c_str()));\n";
            ret += "}\n";

        } else {
            ret += my_name + (getPname().equals("") ? "" : "_" + getPname()) + "_data["+m_c+"] = ";
            ret += "cppArray" + my_name + (getPname().equals("") ? "" : "_" + getPname());
            for(int count = 0; count < temp_dimensions.size(); ++count)
            {
                ret += "["+(char)(((int)c) + 1)+"]";
                c = (char)(((int)c) + 1);
            }
            ret += ";";
        }
        return ret;
    }

    public List<String> getForListGetEnd(String my_name)
    {
        List<String> allMembers = new ArrayList<String>();
        char c = m_c;
        String ret = "";
        List<String> temp_dimensions = getDimensionsWithAlias();
        for(int count = 0; count < temp_dimensions.size(); ++count)
        {
            ret += "}\n";
            if (count == 0) {
                if (!getInnerContentTypeCode().isIsType_d()) {
                    ret += "env->Release"+getInnerContentTypeCode().getJavaUpperFirstTypename()+"ArrayElements("+c+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", " + my_name+ (getPname().equals("") ? "" : "_" + getPname()) + "_data, 0);\n";
                }
            }
            if ((int)c > 'b') {
                ret += "env->SetObjectArrayElement("+(char)(((int)c) - 1)+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+", "+(char)(((int)c) - 1)+", "+c+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+");\n";
                ret += "env->DeleteLocalRef("+c+"Array_"+my_name+ (getPname().equals("") ? "" : "_" + getPname())+");\n";
            }

            allMembers.add(ret);
            c = (char)(((int)c) - 1);
            ret = "";
        }

        return allMembers;
    }

    public boolean isObjectArray()
    {
        return (m_dimensions.size() > 1) || !(getContentTypeCode().isPrimitive());
    }

    public int getDimensionsSize()
    {
        return m_dimensions.size();
    }

    public String getDimensionsInXml()
    {
        String ret = "";
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            ret += ",";
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                ret += m_cont_map.get(m_dimensions.get(count));
            } else {
                ret += m_dimensions.get(count);
            }
        }
        if (ret.length() >= 1) {
            ret = ret.substring(1);
        }
        return ret;
    }

    public void setUseBBMode(boolean useBBMode) {
        useBBMode_ = useBBMode;
    }

    @Override
    public boolean isUseBBMode() {
        return useBBMode_;
    }

    public boolean isIsInnerPlain()
    {
        if (getContentTypeCode() != null && getContentTypeCode().isSetLength()) {
            return true;
        }
        return false;
    }

    @Override
    public boolean isSetLength()
    {
        if(getContentTypeCode() != null && !getContentTypeCode().isSetLength()) {
            return false;
        }
        return true;
    }
    /*public Pair<Integer, Integer> getMaxSerializedSize(int currentSize, int lastDataAligned)
    {
        int lcontainTypeSize = getNoAliasContentTypeCode().getSize();
        int lcontainTypeAlign = 0;
        int larraySize = 1;

        // Element contained type.
        if(lcontainTypeSize > 4)
        {
            lcontainTypeAlign = (lcontainTypeSize - (currentSize % lcontainTypeSize)) & (lcontainTypeSize - 1);
        }

        // Calculate array size.
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            larraySize *= Integer.parseInt(m_dimensions.get(count));
        }

        return new Pair<Integer, Integer>(currentSize + lcontainTypeAlign + (larraySize *  lcontainTypeSize), lcontainTypeSize);
    }

    public int getMaxSerializedSizeWithoutAlignment(int currentSize)
    {
        int lcontainTypeSize = getNoAliasContentTypeCode().getSize();
        int larraySize = 1;

        // Calculate array size.
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            larraySize *= Integer.parseInt(m_dimensions.get(count));
        }

        return currentSize + (larraySize * lcontainTypeSize);
    }*/

//-------------maxSerializedSize----------------//
    @Override
    public long maxSerializedSize(
            long current_alignment)
    {
        long initial_alignment = current_alignment;

        if (!getContentTypeCode().isPrimitive() &&
            !getContentTypeCode().isIsType_c() /*enum*/)
        {
            // DHEADER if XCDRv2
            current_alignment += 4 + TypeCode.cdr_alignment(current_alignment, 4);
        }

        long size = 1;
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                size *= m_cont_map.get(m_dimensions.get(count));
            } else {
                size *= evaluateMathExpression(m_dimensions.get(count));
            }
        }

        if (0 < size)
        {
            current_alignment += ((TypeCode)getContentTypeCode()).maxSerializedSize(current_alignment);

            if (1 < size)
            {
                long element_size_after_first = ((TypeCode)getContentTypeCode()).maxSerializedSize(current_alignment);
                current_alignment += element_size_after_first * (size - 1);
            }
        }

        return current_alignment - initial_alignment;
    }

    @Override
    public long maxPlainTypeSerializedSize(
            long current_alignment,
            long align64) throws RuntimeGenerationException
    {
        long initial_alignment = current_alignment;

        long size = 1;
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                size *= m_cont_map.get(m_dimensions.get(count));
            } else {
                size *= evaluateMathExpression(m_dimensions.get(count));
            }
        }

        if (0 < size)
        {
            current_alignment += ((TypeCode)getContentTypeCode()).maxPlainTypeSerializedSize(
                    current_alignment, align64);

            if (1 < size)
            {
                long element_size_after_first = ((TypeCode)getContentTypeCode()).maxPlainTypeSerializedSize(
                        current_alignment, align64);
                current_alignment += element_size_after_first * (size - 1);
            }
        }

        return current_alignment - initial_alignment;
    }

    public boolean isNotZeroArray()
    {
        long size = 1;
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                size *= m_cont_map.get(m_dimensions.get(count));
            } else {
                size *= evaluateMathExpression(m_dimensions.get(count));
            }
        }
        return 0 != size;
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
        String array_size_string = "";
        for(int count = 0; count < m_dimensions.size(); ++count)
        {
            if(m_dimensions.get(count) != null && !Pattern.matches("\\d+", m_dimensions.get(count))) {
                array_size_string += "[" + m_cont_map.get(m_dimensions.get(count)) + "]";
            } else {
                array_size_string += "[" + m_dimensions.get(count) + "]";
            }
        }
        return inner_hash_string + array_size_string;
    }

    private List<String> m_dimensions;

    private Map<String, Integer> m_cont_map;
    private String m_pname;
    private char m_c;
    private boolean useBBMode_;
}
