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

public class Include
{
	private ArrayList<Doc> m_docs = null;
	private String m_href = null;

	public Include(String href)
	{
		m_href = href;
		if(m_href == null) m_href = "";
		m_docs = new ArrayList<Doc>();
	}

	public void add(Doc doc)
	{
		m_docs.add(doc);
	}

	public String getHref() {
		return m_href;
	}
}
