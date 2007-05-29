/**
 * =========================================================================
 * File        : ogl_shader.cpp
 * Project     : 0 A.D.
 * Description : load and link together shaders; provides hotload support.
 * =========================================================================
 */

#include "precompiled.h"
#include "ogl_shader.h"

#include "lib/res/res.h"
#include "lib/ogl.h"

#include "ps/CLogger.h"
#include "ps/XML/Xeromyces.h"


#define LOG_CATEGORY "shaders"


ERROR_ASSOCIATE(ERR::SHDR_CREATE, "Shader creation failed", -1);
ERROR_ASSOCIATE(ERR::SHDR_COMPILE, "Shader compile failed", -1);
ERROR_ASSOCIATE(ERR::SHDR_NO_SHADER, "Invalid shader reference", -1);
ERROR_ASSOCIATE(ERR::SHDR_LINK, "Shader linking failed", -1);
ERROR_ASSOCIATE(ERR::SHDR_NO_PROGRAM, "Invalid shader program reference", -1);


// Convert a shader object type into a descriptive string.
// If the type enum is not known, the given buffer is used as scratch space
// to format the type number. If buf is null, a generic string is returned.
static const char* shader_type_to_string(GLenum type, char* buf, size_t buflen)
{
	switch(type)
	{
	case GL_VERTEX_SHADER_ARB: return "VERTEX_SHADER";
	case GL_FRAGMENT_SHADER_ARB: return "FRAGMENT_SHADER";
	}
	
	if (!buf)
		return "unknown type enum";
	
	snprintf(buf, buflen, "%u", type);
	return buf;
}

// Return the OpenGL shader type enum for the given string,
// or 0 if the shader type is not known.
static GLenum string_to_shader_type(const char* name)
{
	if (!strcasecmp(name, "VERTEX_SHADER"))
		return GL_VERTEX_SHADER_ARB;
	if (!strcasecmp(name, "FRAGMENT_SHADER"))
		return GL_FRAGMENT_SHADER_ARB;
	return 0;
}


//----------------------------------------------------------------------------
// Handle type implementation

// Data for an Ogl_Shader object
struct Ogl_Shader {
	// Type of shader (e.g. GL_VERTEX_SHADER_ARB)
	GLenum type;

	// ID of the OpenGL shader object
	GLhandleARB id;
};


H_TYPE_DEFINE(Ogl_Shader);


// One-time initialization, called once by h_alloc, which is
// in turn called by ogl_shader_load
static void Ogl_Shader_init(Ogl_Shader* shdr, va_list args)
{
	shdr->type = va_arg(args, GLenum);
}


// Reload the shader object from the source file.
//
// TODO: The OpenGL specification says that all changes to shader objects
// have absolutely no effect on a program object that contains these shaders
// when the program object is already linked.
// So, how can we inform the "parent object" (i.e. the program object) of our change?
static LibError Ogl_Shader_reload(Ogl_Shader* shdr, const char* filename, Handle UNUSED(h))
{
	LibError err  = ERR::FAIL;

	if (shdr->id)
		return INFO::OK;

	FileIOBuf file;
	size_t file_size;
	GLint log_length;
	GLint compile_success;

	RETURN_ERR(vfs_load(filename, file, file_size));

	ogl_WarnIfError();

	shdr->id = pglCreateShaderObjectARB(shdr->type);
	if (!shdr->id)
	{
		// May be out of memory, but bad shdr->type is also possible.
		// In any case, checking OpenGL error state will help spot
		// bad code.
		ogl_WarnIfError();
		
		err = ERR::SHDR_CREATE;
		goto fail_fileloaded;
	}
	
	{
		const GLchar* strings[] = { (const GLchar*)file };
		pglShaderSourceARB(shdr->id, 1, strings, (const GLint*)&file_size);
		pglCompileShaderARB(shdr->id);
	}
	
	pglGetObjectParameterivARB(shdr->id, GL_OBJECT_COMPILE_STATUS_ARB, &compile_success);
	pglGetObjectParameterivARB(shdr->id, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
	if (log_length > 1)
	{
		char typenamebuf[32];
		char* infolog = new char[log_length];
		
		pglGetInfoLogARB(shdr->id, log_length, 0, infolog);
	
		debug_printf("Compile log for shader %hs (type %hs):\n%hs",
			     filename,
			     shader_type_to_string(shdr->type, typenamebuf, ARRAY_SIZE(typenamebuf)),
			     infolog);
		
		delete[] infolog;
	}

	if (!compile_success)
	{
		// Compilation failure caused by syntax errors and similar
		// errors at the GLSL level does not set OpenGL error state
		// according to the spec, but this might still prove to be
		// useful some time.
		ogl_WarnIfError();
	
		char typenamebuf[32];
		debug_printf("Failed to compile shader %hs (type %hs)\n",
			     filename,
			     shader_type_to_string(shdr->type, typenamebuf, ARRAY_SIZE(typenamebuf)));
		
		err  = ERR::SHDR_COMPILE;
		goto fail_shadercreated;
	}

	(void)file_buf_free(file);
	return INFO::OK;

fail_shadercreated:
	pglDeleteObjectARB(shdr->id);
	shdr->id = 0;
fail_fileloaded:
	(void)file_buf_free(file);
	return err;
}


// Free associated resources
static void Ogl_Shader_dtor(Ogl_Shader* shdr)
{
	// shdr->id is 0 when reload has failed
	if (shdr->id)
	{
		pglDeleteObjectARB(shdr->id);
		shdr->id = 0;
	}
}

static LibError Ogl_Shader_validate(const Ogl_Shader* UNUSED(shdr))
{
	// TODO
	return INFO::OK;
}

static LibError Ogl_Shader_to_string(const Ogl_Shader* UNUSED(shdr), char* buf)
{
	snprintf(buf, H_STRING_LEN, "-");
	return INFO::OK;
}


//----------------------------------------------------------------------------
// Public API

// Create, load and compile a shader object of the given type
// (e.g. GL_VERTEX_SHADER_ARB). The given file will be used as
// source code for the shader.
Handle ogl_shader_load(const char* fn, GLenum type)
{
	return h_alloc(H_Ogl_Shader, fn, 0, type);
}


// Free all resources associated with the given handle (subject
// to refcounting).
void ogl_shader_free(Handle& h)
{
	h_free(h, H_Ogl_Shader);
}

// Attach a shader to the given OpenGL program.
LibError ogl_shader_attach(GLhandleARB program, Handle& h)
{
	H_DEREF(h, Ogl_Shader, shdr);

	if (!shdr->id)
		WARN_RETURN(ERR::SHDR_NO_SHADER);

	pglAttachObjectARB(program, shdr->id);

	return INFO::OK;
}



//----------------------------------------------------------------------------
// Program type implementation

struct Ogl_Program {
	// ID of the OpenGL program object
	GLhandleARB id;
};


H_TYPE_DEFINE(Ogl_Program);


// One-time initialization, called once by h_alloc, which is
// in turn called by ogl_program_load
static void Ogl_Program_init(Ogl_Program* UNUSED(p), va_list UNUSED(args))
{
}


// Load the shader associated with one Shader element,
// and attach it to our program object.
static LibError do_load_shader(
		Ogl_Program* p, const char* filename, Handle UNUSED(h),
		const CXeromyces& XeroFile, const XMBElement& Shader)
{
#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	AT(type);
#undef AT
	
	CStr Type = Shader.GetAttributes().GetNamedItem(at_type);

	if (Type.empty())
	{
		LOG(ERROR, LOG_CATEGORY, "%hs: Missing attribute \"type\" in element \"Shader\".",
		    filename);
		WARN_RETURN(ERR::CORRUPTED);
	}

	GLenum shadertype = string_to_shader_type(Type.c_str());
	
	if (!shadertype)
	{
		LOG(ERROR, LOG_CATEGORY, "%hs: Unknown shader type \"%hs\" (valid are: VERTEX_SHADER, FRAGMENT_SHADER).",
		    filename, Type.c_str());
		WARN_RETURN(ERR::CORRUPTED);
	}

	CStr Name = Shader.GetText();
	
	if (Name.empty())
	{
		LOG(ERROR, LOG_CATEGORY, "%hs: Missing shader name.", filename);
		WARN_RETURN(ERR::CORRUPTED);
	}
	
	Handle hshader = ogl_shader_load(Name.c_str(), shadertype);
	RETURN_ERR(hshader);

	ogl_shader_attach(p->id, hshader);

	// According to the OpenGL specification, a shader object's deletion
	// will not be final as long as the shader object is attached to a
	// container object.
	// TODO: How will this work with automatic reload?
	ogl_shader_free(hshader);

	return INFO::OK;
}


// Reload the program object from the source file.
static LibError Ogl_Program_reload(Ogl_Program* p, const char* filename, Handle h)
{
	if (p->id)
		return INFO::OK;

	ogl_WarnIfError();
	
	p->id = pglCreateProgramObjectARB();
	if (!p->id)
	{
		// The spec doesn't mention any error state that can be set
		// here, but it may still help spot bad code.
		ogl_WarnIfError();
		
		WARN_RETURN(ERR::SHDR_CREATE);
	}
	
	CXeromyces XeroFile;
	if (XeroFile.Load(filename) != PSRETURN_OK)
		WARN_RETURN(ERR::CORRUPTED); // more informative error message?

	// Define all the elements and attributes used in the XML file
#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	EL(program);
	EL(shaders);
	EL(shader);
#undef EL

	XMBElement Root = XeroFile.GetRoot();

	if (Root.GetNodeName() != el_program)
	{
		LOG(ERROR, LOG_CATEGORY, "%hs: XML root was not \"Program\".", filename);
		WARN_RETURN(ERR::CORRUPTED);
	}

	XMBElementList RootChildren = Root.GetChildNodes();

	for(int i = 0; i < RootChildren.Count; ++i)
	{
		XMBElement Child = RootChildren.Item(i);
	
		int ChildName = Child.GetNodeName();
		if (ChildName == el_shaders)
		{
			XMBElementList Shaders = Child.GetChildNodes();
			
			for(int j = 0; j < Shaders.Count; ++j)
			{
				XMBElement Shader = Shaders.Item(j);
				
				if (Shader.GetNodeName() != el_shader)
				{
					LOG(ERROR, LOG_CATEGORY, "%hs: Only \"Shader\" may be child of \"Shaders\".",
					    filename);
					WARN_RETURN(ERR::CORRUPTED);
				}
				
				RETURN_ERR(do_load_shader(p, filename, h, XeroFile, Shader));
			}
		}
		else
		{
			LOG(WARNING, LOG_CATEGORY, "%hs: Unknown child of \"Program\".", filename);
		}
	}

	pglLinkProgramARB(p->id);

	GLint log_length;
	GLint linked;
	
	pglGetObjectParameterivARB(p->id, GL_OBJECT_LINK_STATUS_ARB, &linked);
	pglGetObjectParameterivARB(p->id, GL_OBJECT_INFO_LOG_LENGTH_ARB, &log_length);
	if (log_length > 1)
	{
		char* infolog = new char[log_length];
		pglGetInfoLogARB(p->id, log_length, 0, infolog);

		debug_printf("Linker log for %hs:\n%hs\n", filename, infolog);
		delete[] infolog;
	}

	if (!linked)
	{
		debug_printf("Link failed for %hs\n", filename);
		WARN_RETURN(ERR::SHDR_LINK);
	}

	return INFO::OK;
}


// Free associated resources
static void Ogl_Program_dtor(Ogl_Program* p)
{
	if (p->id)
	{
		pglDeleteObjectARB(p->id);
		p->id = 0;
	}
}

static LibError Ogl_Program_validate(const Ogl_Program* UNUSED(p))
{
	// TODO
	return INFO::OK;
}

static LibError Ogl_Program_to_string(const Ogl_Program* UNUSED(p), char* buf)
{
	snprintf(buf, H_STRING_LEN, "-");
	return INFO::OK;
}


//----------------------------------------------------------------------------
// Public API

// Load a program object based on the given XML file description.
// Shader objects are loaded and attached automatically.
Handle ogl_program_load(const char* fn)
{
	return h_alloc(H_Ogl_Program, fn, 0);
}

// Free all resources associated with the given program handle.
void ogl_program_free(Handle& h)
{
	h_free(h, H_Ogl_Program);
}


// Activate the program (glUseProgramObjectARB).
// h may be 0, in which case program objects are disabled.
LibError ogl_program_use(Handle h)
{
	if (!h)
	{
		pglUseProgramObjectARB(0);
		return INFO::OK;
	}

	Ogl_Program* p = H_USER_DATA(h, Ogl_Program);
	if (!p || !p->id)
	{
		pglUseProgramObjectARB(0);
		WARN_RETURN(ERR::INVALID_HANDLE);
	}

	pglUseProgramObjectARB(p->id);
	return INFO::OK;
}


// Query uniform information
GLint ogl_program_get_uniform_location(Handle h, const char* name)
{
	H_DEREF(h, Ogl_Program, p);
	
	return pglGetUniformLocationARB(p->id, name);
}


// Query vertex attribute information
GLint ogl_program_get_attrib_location(Handle h, const char* name)
{
	H_DEREF(h, Ogl_Program, p);
	
	return pglGetAttribLocationARB(p->id, name);
}
