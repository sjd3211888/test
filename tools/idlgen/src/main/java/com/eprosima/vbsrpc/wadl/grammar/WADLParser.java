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

// $ANTXR : "wadl.antxr" -> "WADLParser.java"$
// GENERATED CODE - DO NOT EDIT!

    package com.eprosima.vbsrpc.wadl.grammar;
    import com.eprosima.vbsrpc.wadl.tree.*;
    import java.lang.StringBuilder;
    import java.util.Iterator;
    import com.javadude.antxr.scanner.Attribute;

import com.javadude.antxr.TokenBuffer;
import com.javadude.antxr.TokenStreamException;
import com.javadude.antxr.TokenStreamIOException;
import com.javadude.antxr.ANTXRException;
import com.javadude.antxr.LLkParser;
import com.javadude.antxr.Token;
import com.javadude.antxr.TokenStream;
import com.javadude.antxr.RecognitionException;
import com.javadude.antxr.NoViableAltException;
import com.javadude.antxr.MismatchedTokenException;
import com.javadude.antxr.SemanticException;
import com.javadude.antxr.ParserSharedInputState;
import com.javadude.antxr.collections.impl.BitSet;

// ANTXR XML Mode Support
import com.javadude.antxr.scanner.XMLToken;
import com.javadude.antxr.scanner.Attribute;
import java.util.Map;
import java.util.HashMap;

public class WADLParser extends com.javadude.antxr.LLkParser       implements WADLParserTokenTypes
 {
	// ANTXR XML Mode Support
	private static Map __xml_namespaceMap = new HashMap();
	public static Map getNamespaceMap() {return __xml_namespaceMap;}
	public static String resolveNamespace(String prefix) {
		if (prefix == null || "".equals(prefix))
			return "";
		return (String)__xml_namespaceMap.get(prefix);
	}
	static {
		__xml_namespaceMap.put("$DEFAULT","http://wadl.dev.java.net/2009/02");
 }

protected WADLParser(TokenBuffer tokenBuf, int k) {
  super(tokenBuf,k);
  tokenNames = _tokenNames;
}

public WADLParser(TokenBuffer tokenBuf) {
  this(tokenBuf,1);
}

protected WADLParser(TokenStream lexer, int k) {
  super(lexer,k);
  tokenNames = _tokenNames;
}

public WADLParser(TokenStream lexer) {
  this(lexer,1);
}

public WADLParser(ParserSharedInputState state) {
  super(state,1);
  tokenNames = _tokenNames;
}

	public final Application  document() throws RecognitionException, TokenStreamException {
		Application application = null;

		try {      // for error handling
			application=__xml_application();
			match(Token.EOF_TYPE);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_0);
		}
		return application;
	}

	public final Application  __xml_application() throws RecognitionException, TokenStreamException {
		Application application = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(4);
			{

			// TODO XXX SEMANTIC VALIDATION

			application = new Application(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"targetNamespace"));
			Grammar grammar = null;
			Resources resources = null;
			Method method = null;
			Param param = null;
			Doc doc = null;
			Representation representation = null;
			ResourceType resource_type = null;

			{
			_loop5:
			do {
				if ((LA(1)==6)) {
					doc=__xml_doc();
					application.add(doc);
				}
				else {
					break _loop5;
				}

			} while (true);
			}
			{
			switch ( LA(1)) {
			case 8:
			{
				grammar=__xml_grammars();
				application.setGrammar(grammar);
				break;
			}
			case XML_END_TAG:
			case 9:
			case 11:
			case 13:
			case 16:
			case 17:
			{
				break;
			}
			default:
			{
				throw new NoViableAltException(LT(1), getFilename());
			}
			}
			}
			{
			_loop8:
			do {
				if ((LA(1)==9)) {
					resources=__xml_resources();
					application.add(resources);
				}
				else {
					break _loop8;
				}

			} while (true);
			}
			{
			_loop10:
			do {
				switch ( LA(1)) {
				case 17:
				{
					resource_type=__xml_resource_type();
					application.add(resource_type);
					break;
				}
				case 11:
				{
					method=__xml_method();
					application.add(method);
					break;
				}
				case 16:
				{
					representation=__xml_representation();
					application.add(representation);
					break;
				}
				case 13:
				{
					param=__xml_param();
					application.add(param);
					break;
				}
				default:
				{
					break _loop10;
				}
				}
			} while (true);
			}

			application.updateStructure();

			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_0);
		}
		return application;
	}

	public final Doc  __xml_doc() throws RecognitionException, TokenStreamException {
		Doc doc = null;;

		Token  __xml_startTag = null;
		Token  value = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(6);
			{

			doc = new Doc(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"title"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"ref"));
			String text = null;

			value = LT(1);
			match(PCDATA);
			doc.add(value.getText());
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_1);
		}
		return doc;
	}

	public final Grammar  __xml_grammars() throws RecognitionException, TokenStreamException {
		Grammar grammar = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(8);
			{

			grammar = new Grammar();
			Include include = null;
			Doc doc = null;

			{
			_loop16:
			do {
				if ((LA(1)==6)) {
					doc=__xml_doc();
					grammar.add(doc);
				}
				else {
					break _loop16;
				}

			} while (true);
			}
			{
			_loop18:
			do {
				if ((LA(1)==12)) {
					include=__xml_include();
					grammar.add(include);
				}
				else {
					break _loop18;
				}

			} while (true);
			}
			{
			switch ( LA(1)) {
			case OTHER_TAG:
			{
				StringBuilder buffer = new StringBuilder();
				schemaTag(buffer);
				grammar.setLaxTex(buffer.toString());
				break;
			}
			case XML_END_TAG:
			{
				break;
			}
			default:
			{
				throw new NoViableAltException(LT(1), getFilename());
			}
			}
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_2);
		}
		return grammar;
	}

	public final Resources  __xml_resources() throws RecognitionException, TokenStreamException {
		Resources resources = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(9);
			{

			resources = new Resources(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"base"));
			Resource resource = null;
			Doc doc = null;

			{
			_loop23:
			do {
				if ((LA(1)==6)) {
					doc=__xml_doc();
					resources.add(doc);
				}
				else {
					break _loop23;
				}

			} while (true);
			}
			{
			_loop25:
			do {
				if ((LA(1)==10)) {
					resource=__xml_resource();
					resources.add(resource);
				}
				else {
					break _loop25;
				}

			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_2);
		}
		return resources;
	}

	public final ResourceType  __xml_resource_type() throws RecognitionException, TokenStreamException {
		ResourceType resource_type = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(17);
			{

			resource_type = new ResourceType();

			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_3);
		}
		return resource_type;
	}

	public final Method  __xml_method() throws RecognitionException, TokenStreamException {
		Method method = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(11);
			{

			method = new Method(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"id"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"name"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"href"));
			Doc doc = null;
			Request request = null;
			Response response = null;

			{
			_loop33:
			do {
				if ((LA(1)==6)) {
					doc=__xml_doc();
					method.add(doc);
				}
				else {
					break _loop33;
				}

			} while (true);
			}
			{
			switch ( LA(1)) {
			case 14:
			{
				request=__xml_request();
				method.setRequest(request);
				break;
			}
			case XML_END_TAG:
			case 15:
			{
				break;
			}
			default:
			{
				throw new NoViableAltException(LT(1), getFilename());
			}
			}
			}
			{
			_loop36:
			do {
				if ((LA(1)==15)) {
					response=__xml_response();
					method.add(response);
				}
				else {
					break _loop36;
				}

			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_4);
		}
		return method;
	}

	public final Representation  __xml_representation() throws RecognitionException, TokenStreamException {
		Representation representation = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(16);
			{

			representation = new Representation(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"href"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"id"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"mediaType"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"element"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"profile"));

			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_3);
		}
		return representation;
	}

	public final Param  __xml_param() throws RecognitionException, TokenStreamException {
		Param param = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(13);
			{

			param = new Param(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"href"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"name"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"style"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"id"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"type"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"required"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"repeating"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"fixed"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"path"));
			Doc doc = null;

			{
			_loop44:
			do {
				if ((LA(1)==6)) {
					doc=__xml_doc();
					param.add(doc);
				}
				else {
					break _loop44;
				}

			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_4);
		}
		return param;
	}

	public final Include  __xml_include() throws RecognitionException, TokenStreamException {
		Include include = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(12);
			{

			include = new Include(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"href"));
			Doc doc = null;

			{
			_loop40:
			do {
				if ((LA(1)==6)) {
					doc=__xml_doc();
					include.add(doc);
				}
				else {
					break _loop40;
				}

			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_5);
		}
		return include;
	}

	public final void schemaTag(
		StringBuilder buffer
	) throws RecognitionException, TokenStreamException {

		// Take the XML tag.
		String tag = LT(1).getText();
		// Store the XML tag.
		buffer.append("<" + tag);
		// Take the attributes.
		Iterator it = ((XMLToken)LT(1)).getAttributes();
		// Process attributes.
		while(it.hasNext())
		{
		Attribute attr = (Attribute)it.next();
		buffer.append(" " + (attr.getNamespace() != null ? attr.getNamespace() : "") +
		attr.getLocalName() + "=\"" + attr.getValue() + "\"");
		}
		// Close the tag.
		buffer.append(">\n");

		try {      // for error handling
			match(OTHER_TAG);
			{
			_loop61:
			do {
				if ((LA(1)==OTHER_TAG)) {
					schemaTag(buffer);
				}
				else {
					break _loop61;
				}

			} while (true);
			}
			match(XML_END_TAG);
			buffer.append("</" + tag + ">\n");
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_6);
		}
	}

	public final Resource  __xml_resource() throws RecognitionException, TokenStreamException {
		Resource resource = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(10);
			{

			resource = new Resource(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"id"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"type"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"queryType"), ((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"path"));
			Param param = null;
			Resource resourceChild = null;
			Method method = null;
			Doc doc = null;

			{
			_loop29:
			do {
				switch ( LA(1)) {
				case 6:
				{
					doc=__xml_doc();
					resource.add(doc);
					break;
				}
				case 13:
				{
					param=__xml_param();
					resource.add(param);
					break;
				}
				case 11:
				{
					method=__xml_method();
					method.setParentResource(resource); resource.add(method);
					break;
				}
				case 10:
				{
					resourceChild=__xml_resource();
					resourceChild.setParentResource(resource); resource.add(resourceChild);
					break;
				}
				default:
				{
					break _loop29;
				}
				}
			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_7);
		}
		return resource;
	}

	public final Request  __xml_request() throws RecognitionException, TokenStreamException {
		Request request = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(14);
			{

			request = new Request();
			Param param = null;
			Representation representation = null;

			{
			_loop48:
			do {
				if ((LA(1)==16)) {
					representation=__xml_representation();
					request.add(representation);
				}
				else {
					break _loop48;
				}

			} while (true);
			}
			{
			_loop50:
			do {
				if ((LA(1)==13)) {
					param=__xml_param();
					request.add(param);
				}
				else {
					break _loop50;
				}

			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_8);
		}
		return request;
	}

	public final Response  __xml_response() throws RecognitionException, TokenStreamException {
		Response response = null;

		Token  __xml_startTag = null;

		try {      // for error handling
			__xml_startTag = LT(1);
			match(15);
			{

			response = new Response(((XMLToken)__xml_startTag).getAttribute(resolveNamespace(""),"status"));
			Representation representation = null;

			{
			_loop54:
			do {
				if ((LA(1)==16)) {
					representation=__xml_representation();
					response.add(representation);
				}
				else {
					break _loop54;
				}

			} while (true);
			}
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_8);
		}
		return response;
	}

	public final void otherTag() throws RecognitionException, TokenStreamException {

		try {      // for error handling
			match(OTHER_TAG);
			{
			_loop64:
			do {
				switch ( LA(1)) {
				case OTHER_TAG:
				{
					otherTag();
					break;
				}
				case PCDATA:
				{
					match(PCDATA);
					break;
				}
				default:
				{
					break _loop64;
				}
				}
			} while (true);
			}
			match(XML_END_TAG);
		}
		catch (RecognitionException ex) {
			reportError(ex);
			recover(ex,_tokenSet_9);
		}
	}

	public static final String[] _tokenNames = {
		"<0>",
		"EOF",
		"<2>",
		"NULL_TREE_LOOKAHEAD",
		"\"<application>\"",
		"XML_END_TAG",
		"\"<doc>\"",
		"PCDATA",
		"\"<grammars>\"",
		"\"<resources>\"",
		"\"<resource>\"",
		"\"<method>\"",
		"\"<include>\"",
		"\"<param>\"",
		"\"<request>\"",
		"\"<response>\"",
		"\"<representation>\"",
		"\"<resource_type>\"",
		"OTHER_TAG"
	};

	private static final long[] mk_tokenSet_0() {
		long[] data = { 2L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_0 = new BitSet(mk_tokenSet_0());
	private static final long[] mk_tokenSet_1() {
		long[] data = { 524128L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_1 = new BitSet(mk_tokenSet_1());
	private static final long[] mk_tokenSet_2() {
		long[] data = { 207392L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_2 = new BitSet(mk_tokenSet_2());
	private static final long[] mk_tokenSet_3() {
		long[] data = { 206880L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_3 = new BitSet(mk_tokenSet_3());
	private static final long[] mk_tokenSet_4() {
		long[] data = { 207968L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_4 = new BitSet(mk_tokenSet_4());
	private static final long[] mk_tokenSet_5() {
		long[] data = { 266272L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_5 = new BitSet(mk_tokenSet_5());
	private static final long[] mk_tokenSet_6() {
		long[] data = { 262176L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_6 = new BitSet(mk_tokenSet_6());
	private static final long[] mk_tokenSet_7() {
		long[] data = { 11360L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_7 = new BitSet(mk_tokenSet_7());
	private static final long[] mk_tokenSet_8() {
		long[] data = { 32800L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_8 = new BitSet(mk_tokenSet_8());
	private static final long[] mk_tokenSet_9() {
		long[] data = { 262304L, 0L};
		return data;
	}
	public static final BitSet _tokenSet_9 = new BitSet(mk_tokenSet_9());

	}
