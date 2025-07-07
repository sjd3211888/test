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
// 2023-05 ~ 2025-05: idlgen optimization: pubsub mode java/jni code generation, java typedef, typeobject fix, ContainerPrintHelpers removal, etc.
// ------------------------------------------------------------------

package com.eprosima.idl.parser.tree;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import com.eprosima.idl.parser.tree.Param;
import com.eprosima.idl.parser.typecode.TypeCode;
import org.antlr.v4.runtime.Token;

public class Interface extends ExportContainer implements Definition
{
    public Interface(String scopeFile, boolean isInScope, String scope, String name, Token tk)
    {
        super(scopeFile, isInScope, scope, name, tk);
        m_bases = new HashMap<String, Interface>();
    }

    public Interface(Interface other) {
        super(other);
        m_bases = new HashMap<String, Interface>();
        for (Map.Entry<String, Interface> entry : other.m_bases.entrySet()) {
            if (entry.getValue() != this) {
                this.m_bases.put(entry.getKey(), new Interface(entry.getValue()));
            } else {
                this.m_bases.put(entry.getKey(), this);
            }
        }
        this.m_parent = other.m_parent;

    }

    @Override
    public Interface copy() {
        return new Interface(this);
    }

    public void setParent(Object obj)
    {
        m_parent = obj;
    }

    public Object getParent()
    {
        return m_parent;
    }

    @Override
    public boolean isIsModule()
    {
        return false;
    }

    @Override
    public boolean isIsInterface()
    {
        return true;
    }

    @Override
    public boolean isIsException()
    {
        return false;
    }

    @Override
    public boolean isIsTypeDeclaration()
    {
        return false;
    }

    @Override
    public boolean isIsConstDeclaration()
    {
        return false;
    }

	@Override
    public boolean isIsAnnotation()
    {
        return false;
    }

    public boolean addBase(Interface interf)
    {
        Interface prev = m_bases.put(interf.getName(), interf);

        if(prev != null)
            return false;

        return true;
    }

    public ArrayList<Interface> getBases()
    {
        return new ArrayList<Interface>(m_bases.values());
    }

    public Map<String, Interface> getBasesMap()
    {
        return m_bases;
    }

    /*!
     * @brief This function returns the exception defined inside the interface.
     */
    public Exception getException(String currentScope, String ename)
    {
        com.eprosima.idl.parser.tree.Exception exception = null;

        for(int count = 0; exception == null && count < getExports().size(); ++count)
        {
    	    int lastIndex = -1;

    	    if(getExports().get(count).isIsException())
    		{
                String tmpname = ((com.eprosima.idl.parser.tree.Exception)getExports().get(count)).getScopedname();

                if(tmpname.equals(ename))
                {
                    exception = (com.eprosima.idl.parser.tree.Exception)getExports().get(count);
                }
                else
                {
                	// Probar si no tiene scope, con el scope actual.
                    if(exception == null && ((lastIndex = ename.lastIndexOf("::")) == -1) &&
                    	    tmpname.equals(currentScope + ename))
                    {
                        exception = (com.eprosima.idl.parser.tree.Exception)getExports().get(count);
                    }
                }
    		}
        }

        return exception;
    }

    /*!
     * @brief This function returns all operations of the interface.
     * This function is used in the string templates.
     */
    public ArrayList<Operation> getOperations()
    {
        if(m_operations == null)
        {
            m_operations = new ArrayList<Operation>();

            // Get own operations.
            for(int count = 0; count < getExports().size(); ++count)
            {
                if(getExports().get(count).isIsOperation())
                {
                    m_operations.add((Operation)getExports().get(count));
                }
            }
        }

        return m_operations;
    }

    public ArrayList<Operation> getAll_operations()
    {
        if(m_all_operations == null)
        {
            m_all_operations = new ArrayList<Operation>();

            // Get parent operations.
            for(Interface iface : m_bases.values())
            {
                m_all_operations.addAll(iface.getAll_operations());
            }

            // Get own operations.
            for(int count = 0; count < getExports().size(); ++count)
            {
                if(getExports().get(count).isIsOperation())
                {
                    m_all_operations.add((Operation)getExports().get(count));
                }
            }
        }

        return m_all_operations;
    }

    public ArrayList<String> getAllOperationsStruct()
    {
        if(m_all_operations == null)
        {
            m_all_operations = new ArrayList<Operation>();

            // Get parent operations.
            for(Interface iface : m_bases.values())
            {
                m_all_operations.addAll(iface.getAll_operations());
            }

            // Get own operations.
            for(int count = 0; count < getExports().size(); ++count)
            {
                if(getExports().get(count).isIsOperation())
                {
                    m_all_operations.add((Operation)getExports().get(count));
                }
            }
        }

        Set<String> uniqueStrings = new HashSet<>();

        for (Operation operation : m_all_operations) {
            for(Param param : operation.getParameters()) {
                if (param.getTypecode().isIsType_10()) {
                    uniqueStrings.add(param.getTypecode().getContentTypeCode().getCScopednamenormal());
                } else {
                    uniqueStrings.add(param.getTypecode().getCScopednamenormal());
                }

            }
        }
        ArrayList<String> uniqueList = new ArrayList<>(uniqueStrings);
        return uniqueList;
    }

    public ArrayList<String> getAllOperationsStructWithNull()
    {
        if(m_all_operations == null)
        {
            m_all_operations = new ArrayList<Operation>();

            // Get parent operations.
            for(Interface iface : m_bases.values())
            {
                m_all_operations.addAll(iface.getAll_operations());
            }

            // Get own operations.
            for(int count = 0; count < getExports().size(); ++count)
            {
                if(getExports().get(count).isIsOperation())
                {
                    m_all_operations.add((Operation)getExports().get(count));
                }
            }
        }

        Set<String> uniqueStrings = new HashSet<>();

        for (Operation operation : m_all_operations) {
            for(Param param : operation.getParameters()) {
                uniqueStrings.add(param.getTypecode().getCScopednamenormal());
            }
        }
        ArrayList<String> uniqueList = new ArrayList<>(uniqueStrings);
        return uniqueList;
    }

    public boolean isReftype()
    {
        ArrayList<Operation> al = new ArrayList<Operation>();
        al = getAll_operations();
        for(int count = 0; count < al.size(); ++count) {
            ArrayList<Param> temp_param = al.get(count).getInputparam();
            for(int count_p = 0; count_p < temp_param.size(); ++count_p)
                if(temp_param.get(count_p).isInput() && temp_param.get(count_p).isOutput())
                    return true;
        }
        return false;
    }

    /*!
     * @brief This function is used in stringtemplates to not generate module in some cases (Right now in generated (previous c) idl).
     */
    public boolean isThereAreDeclarations()
    {
        boolean returnedValue = false;

        for(int count = 0; !returnedValue && count < getExports().size(); ++count)
        {
            returnedValue = getExports().get(count).isIsTypeDeclaration() ||
                getExports().get(count).isIsConstDeclaration() || getExports().get(count).isIsException();
        }

        return returnedValue;
    }
    private String computeRelativePath(String fromNamespace, String toNamespace) {
        fromNamespace = fromNamespace.replaceFirst("^::", "");
        toNamespace = toNamespace.replaceFirst("^::", "");

        if (fromNamespace.equals(toNamespace)) {
            return "";
        }

        String[] fromParts = fromNamespace.isEmpty() ? new String[0] : fromNamespace.split("::");
        String[] toParts = toNamespace.isEmpty() ? new String[0] : toNamespace.split("::");

        int commonLength = 0;
        while (commonLength < fromParts.length && commonLength < toParts.length
                && fromParts[commonLength].equals(toParts[commonLength])) {
            commonLength++;
        }

        int upLevels = fromParts.length - commonLength;

        StringBuilder relativePath = new StringBuilder();

        for (int i = 0; i < upLevels; i++) {

            relativePath.append("super::");
        }

        for (int i = commonLength; i < toParts.length; i++) {

            if (relativePath.length() > 0 && !relativePath.toString().endsWith("::")) {
                relativePath.append("::");
            }
            relativePath.append(toParts[i]);
        }

        String result = relativePath.toString();
        while (result.endsWith("::")) {
            result = result.substring(0, result.length() - 2);
        }

        return result;
    }
    private String generateUseStatement(String paramNamespace, String typeName, com.eprosima.idl.parser.tree.Module paramModule) {
        String fromNamespace = getScope();
        String relativePath = computeRelativePath(fromNamespace, paramNamespace);

        if (relativePath.isEmpty()) {
            return "";
        }

        String importStatement = "use " + relativePath + "::" + typeName + ";";
        return importStatement;
    }

    public void computeRustStatements() {
        m_useStatements = new ArrayList<>();
        for (Operation operation : getOperations()) {
            for (Param param : operation.getParameters()) {
                String paramNamespace = param.getTypecode().getScope();
                if (paramNamespace == null) {
                    paramNamespace = "";
                } else {
                    paramNamespace = paramNamespace.replaceFirst("^::", "");
                }

                TypeCode type = param.getTypecode();
                com.eprosima.idl.parser.tree.Module module = type.ctx.existsModule(paramNamespace);
                if (module != null) {
                    if (!paramNamespace.equals(getScope())) {
                        String importStatement = generateUseStatement(paramNamespace, param.getTypecode().getRustTypename(), module);
                        if (!importStatement.isEmpty() && !m_useStatements.contains(importStatement)) {
                            m_useStatements.add(importStatement);
                        }
                    }
                }
            }
        }
    }

    public boolean isUseStatements() {
        computeRustStatements();
        return !m_useStatements.isEmpty();
    }

    public List<String> getUseStatements() {
        computeRustStatements();
        return m_useStatements;
    }

private String computeRelativePath(String fromNamespace, String toNamespace, int[] hierarchyRelation) {
    if (fromNamespace.equals(toNamespace)) {
        hierarchyRelation[0] = 0;
        return "";
    }

    String[] fromParts = fromNamespace.split("::");
    String[] toParts = toNamespace.split("::");

    int commonLength = 0;
    while (commonLength < fromParts.length && commonLength < toParts.length
            && fromParts[commonLength].equals(toParts[commonLength])) {
        commonLength++;
    }

    int upLevels = fromParts.length - commonLength;
    hierarchyRelation[0] = toParts.length - fromParts.length;

    StringBuilder relativePath = new StringBuilder();
    if (upLevels > 0) {
        for (int i = 0; i < upLevels; i++) {
            relativePath.append("super::");
        }
    }

    if (commonLength == toParts.length && upLevels == 1) {
        relativePath.append(toParts[toParts.length - 1]).append("_type");
    } else {
        for (int i = commonLength; i < toParts.length; i++) {
            relativePath.append(toParts[i]);
            if (i < toParts.length - 1) {
                relativePath.append("::");
            }
        }
    }
    return relativePath.toString();
}

    private Object m_parent = null;

    private List<String> m_useStatements = null;

    //! Contains all interfaces it inherits from.
    private Map<String, Interface> m_bases = null;
    //! Contains all operations.
    private ArrayList<Operation> m_operations = null;
    private ArrayList<Operation> m_all_operations = null;
}
