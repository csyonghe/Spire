#ifndef CORE_LIB_STREAM_H
#define CORE_LIB_STREAM_H

#include "Basic.h"

namespace CoreLib
{
	namespace IO
	{
		using CoreLib::Basic::Exception;
		using CoreLib::Basic::String;
		using CoreLib::Basic::RefPtr;

		class IOException : public Exception
		{
		public:
			IOException()
			{}
			IOException(const String & message)
				: CoreLib::Basic::Exception(message)
			{
			}
		};

		class EndOfStreamException : public IOException
		{
		public:
			EndOfStreamException()
			{}
			EndOfStreamException(const String & message)
				: IOException(message)
			{
			}
		};

		enum class SeekOrigin
		{
			Start, End, Current
		};

		class Stream : public CoreLib::Basic::Object
		{
		public:
			virtual Int64 GetPosition()=0;
			virtual void Seek(SeekOrigin origin, Int64 offset)=0;
			virtual Int64 Read(void * buffer, Int64 length) = 0;
			virtual Int64 Write(const void * buffer, Int64 length) = 0;
			virtual bool IsEnd() = 0;
			virtual bool CanRead() = 0;
			virtual bool CanWrite() = 0;
			virtual void Close() = 0;
		};

		class BinaryReader
		{
		private:
			RefPtr<Stream> stream;
		public:
			BinaryReader(RefPtr<Stream> stream)
			{
				this->stream = stream;
			}
			Stream * GetStream()
			{
				return stream.Ptr();
			}
			void ReleaseStream()
			{
				stream.Release();
			}
			template<typename T>
			void Read(T * buffer, int count)
			{
				stream->Read(buffer, sizeof(T)*(Int64)count);
			}
			int ReadInt32()
			{
				int rs;
				stream->Read(&rs, sizeof(int));
				return rs;
			}
			short ReadInt16()
			{
				short rs;
				stream->Read(&rs, sizeof(short));
				return rs;
			}
			Int64 ReadInt64()
			{
				Int64 rs;
				stream->Read(&rs, sizeof(Int64));
				return rs;
			}
			float ReadFloat()
			{
				float rs;
				stream->Read(&rs, sizeof(float));
				return rs;
			}
			double ReadDouble()
			{
				double rs;
				stream->Read(&rs, sizeof(double));
				return rs;
			}
			char ReadChar()
			{
				char rs;
				stream->Read(&rs, sizeof(char));
				return rs;
			}
			String ReadString()
			{
				int len = ReadInt32();
				char * buffer = new char[len+1];
				try
				{
					stream->Read(buffer, len);
				}
				catch(IOException & e)
				{
					delete [] buffer;
					throw e;
				}
				buffer[len] = 0;
				return String::FromBuffer(buffer, len);
			}
		};

		class BinaryWriter
		{
		private:
			RefPtr<Stream> stream;
		public:
			BinaryWriter(RefPtr<Stream> stream)
			{
				this->stream = stream;
			}
			Stream * GetStream()
			{
				return stream.Ptr();
			}
			template<typename T>
			void Write(const T& val)
			{
				stream->Write(&val, sizeof(T));
			}
			template<typename T>
			void Write(T * buffer, int count)
			{
				stream->Write(buffer, sizeof(T)*(Int64)count);
			}
			void Write(const String & str)
			{
				Write(str.Length());
				Write(str.Buffer(), str.Length());
			}
			void ReleaseStream()
			{
				stream.Release();
			}
			void Close()
			{
				stream->Close();
			}
		};

		enum class FileMode
		{
			Create, Open, CreateNew, Append
		};

		enum class FileAccess
		{
			Read = 1, Write = 2, ReadWrite = 3
		};

		enum class FileShare
		{
			None, ReadOnly, WriteOnly, ReadWrite
		};

		class FileStream : public Stream
		{
		private:
			FILE * handle;
			FileAccess fileAccess;
			bool endReached = false;
			void Init(const CoreLib::Basic::String & fileName, FileMode fileMode, FileAccess access, FileShare share);
		public:
			FileStream(const CoreLib::Basic::String & fileName, FileMode fileMode = FileMode::Open);
			FileStream(const CoreLib::Basic::String & fileName, FileMode fileMode, FileAccess access, FileShare share);
			~FileStream();
		public:
			virtual Int64 GetPosition();
			virtual void Seek(SeekOrigin origin, Int64 offset);
			virtual Int64 Read(void * buffer, Int64 length);
			virtual Int64 Write(const void * buffer, Int64 length);
			virtual bool CanRead();
			virtual bool CanWrite();
			virtual void Close();
			virtual bool IsEnd();
		};
	}
}

#endif
