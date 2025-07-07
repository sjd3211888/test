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

public class Representation {

	private String href;
	private String id;
	private String mediaType;
	private String element;
	private String profile;

	public Representation(String href, String id, String mediaType,
			String element, String profile) {
		this.href = href;
		if(this.href == null)
			this.href = "";
		this.id = id;
		if(this.id == null)
			this.id = "";
		this.mediaType = mediaType;
		if(this.mediaType == null)
			this.mediaType = "";
		this.element = element;
		if(this.element == null)
			this.element = "";
		this.profile = profile;
		if(this.profile == null)
			this.profile = "";
	}

	public String getId() {
		return id;
	}

	public String getElement() {
		return element;
	}

	public String getMediaType() {
		return mediaType;
	}

	public String getHref() {
		return href;
	}

	public void copyFrom(Representation globalRepresentation) {
		id = globalRepresentation.id;
		mediaType = globalRepresentation.mediaType;
		element = globalRepresentation.element;
		profile = globalRepresentation.profile;
	}
}
