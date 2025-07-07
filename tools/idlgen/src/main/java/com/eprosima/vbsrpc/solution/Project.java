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
// 2023-07 ~ 2025-05: idlgen optimization: support for XML string interface, cpp generation fix, RTI idl import and code generation, rpc pubsubtypes dependency, typeobject fix, clean up fast-related traces, template file standardization, etc.
// ------------------------------------------------------------------

package com.eprosima.vbsrpc.solution;

import java.util.ArrayList;
import java.util.LinkedHashSet;

import com.eprosima.solution.GUIDGenerator;

public class Project extends com.eprosima.solution.Project {
	public Project(String name, String file, LinkedHashSet<String> dependencies) {
		super(name, file, dependencies);

		m_clientsrcfiles = new ArrayList<String>();
		m_clientincludefiles = new ArrayList<String>();
		m_serversrcfiles = new ArrayList<String>();
		m_serverincludefiles = new ArrayList<String>();

		m_jniincludefiles = new ArrayList<String>();
		m_jnisrcfiles = new ArrayList<String>();
		m_idlincludefiles = new ArrayList<String>();
        m_idlincludefiles.addAll((LinkedHashSet<String>)dependencies);
	}

	public void addClientSrcFile(String file) {
		m_clientsrcfiles.add(file);
	}

	public ArrayList<String> getClientSrcFiles() {
		return m_clientsrcfiles;
	}

	public void addClientIncludeFile(String file) {
		m_clientincludefiles.add(file);
	}

	public ArrayList<String> getClientIncludeFiles() {
		return m_clientincludefiles;
	}

	public void addServerSrcFile(String file) {
		m_serversrcfiles.add(file);
	}

	public ArrayList<String> getServerSrcFiles() {
		return m_serversrcfiles;
	}

	public void addServerIncludeFile(String file) {
		m_serverincludefiles.add(file);
	}

	public ArrayList<String> getServerIncludeFiles() {
		return m_serverincludefiles;
	}

	public boolean getContainsInterfaces() {
		return m_containsInterfaces;
	}

	public void setContainsInterfaces(boolean containsInterfaces) {
		m_containsInterfaces = containsInterfaces;
	}

	/*
	 * !
	 *
	 * @brief Used in string templates.
	 */
	public String getClientGuid() {
		return GUIDGenerator.genGUID(getFile() + "Client");
	}

	/*
	 * !
	 *
	 * @brief Used in string templates.
	 */
	public String getClientExampleGuid() {
		return GUIDGenerator.genGUID(getFile() + "ClientExample");
	}

	/*
	 * !
	 *
	 * @brief Used in string templates.
	 */
	public String getServerGuid() {
		return GUIDGenerator.genGUID(getFile() + "Server");
	}

	/*
	 * !
	 *
	 * @brief Used in string templates.
	 */
	public String getServerExampleGuid() {
		return GUIDGenerator.genGUID(getFile() + "ServerExample");
	}

	public void addJniIncludeFile(String file) {
		m_jniincludefiles.add(file);
	}

	public ArrayList<String> getJniIncludeFiles() {
		return m_jniincludefiles;
	}

	public void addJniSrcFile(String file) {
		m_jnisrcfiles.add(file);
	}

	public ArrayList<String> getJniSrcFiles() {
		return m_jnisrcfiles;
	}

	public void addIDLIncludeFile(String file)
    {
        m_idlincludefiles.add(file);
    }

    public ArrayList<String> getIDLIncludeFiles()
    {
        return m_idlincludefiles;
    }

	private boolean m_containsInterfaces = false;
	private ArrayList<String> m_clientsrcfiles = null;
	private ArrayList<String> m_clientincludefiles = null;
	private ArrayList<String> m_serversrcfiles = null;
	private ArrayList<String> m_serverincludefiles = null;
	private ArrayList<String> m_jniincludefiles = null;
	private ArrayList<String> m_jnisrcfiles = null;
	private ArrayList<String> m_idlincludefiles = null;
	String m_guid = null;
}
