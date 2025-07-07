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

package com.eprosima.vbsrpc.wadl.tree;

import java.util.ArrayList;

public class Resources
{
    private ArrayList<Resource> m_resources = null;
    private ArrayList<Doc> m_docs = null;
    private String m_base = null;

	public Resources(String base)
	{
		m_base = base;
		if(m_base == null)
			m_base = "";
		m_resources = new ArrayList<Resource>();
		m_docs = new ArrayList<Doc>();
	}

	public String getBase() {
		return m_base;
	}

	public void add(Resource resource)
	{
		m_resources.add(resource);
	}

	public ArrayList<Resource> getResources() {
		return m_resources;
	}

	public void add(Doc doc)
	{
		m_docs.add(doc);
	}

	/*!
	 * @brief Expands reference parameters with global parameters in all child resources
	 *
	 * @param globalParameters
	 */
	public void setGlobalParameters(ArrayList<Param> globalParameters) {
		for(Resource resource: m_resources) {
			resource.setGlobalParameters(globalParameters);
		}
	}

	/*!
	 * @brief Expands reference methods with global methods in all child resources
	 * @param globalMethods
	 */
	public void setGlobalMethods(ArrayList<Method> globalMethods) {
		for(Resource resource: m_resources) {
			resource.setGlobalMethods(globalMethods);
		}
	}

	/*!
	 * @brief Expands reference representations with global representations in all child resources
	 * @param globalRepresentations
	 */
	public void setGlobalRepresentations(ArrayList<Representation> globalRepresentations) {
		for(Resource resource: m_resources) {
			resource.setGlobalRepresentations(globalRepresentations);
		}
	}
}
