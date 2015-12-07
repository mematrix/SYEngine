#ifndef __AKA_MATROSKA_EBML_H
#define __AKA_MATROSKA_EBML_H

#include "AkaMatroskaGlobal.h"
#include "AkaMatroskaIOCallback.h"

namespace AkaMatroska {

	namespace Core {
		int CalcEbmlIdBytes(uint32_t id);
		int CalcEbmlSizeBytes(uint64_t num);
	}

	namespace Ebml {
		bool WriteId(Core::IOCallback* cb, uint32_t id);
		bool WriteSize(Core::IOCallback* cb, uint64_t size, int force_preserve_bytes = 0);
		bool WriteSizeUnknown(Core::IOCallback* cb, int range_bytes = 8);

		void WriteVoid(Core::IOCallback* cb, unsigned free_size);
		struct Master
		{
			Master(Core::IOCallback* cb) throw() // = EbmlVoid.
			{ _callback = cb; InitMaster(0xEC); }
			Master(Core::IOCallback* cb, uint32_t id) throw()
			{ _callback = cb; InitMaster(id); }
			Master(Core::IOCallback* cb, uint32_t id, uint64_t maybe_size) throw()
			{ _callback = cb; InitMaster(id, maybe_size); }
			~Master() throw() { Complete(); }

			bool PutElementUInt(uint32_t eid, uint64_t value);
			bool PutElementSInt(uint32_t eid, int64_t value);
			bool PutElementFloat(uint32_t eid, double value);
			bool PutElementFloat(uint32_t eid, float value)
			{ return PutElementFloat(eid, double(value)); }
			bool PutElementBinary(uint32_t eid, const void* data, unsigned size);
			bool PutElementBArray(uint32_t eid, const void* data[], unsigned size[], unsigned count);
			bool PutElementString(uint32_t eid, const char* str)
			{ return PutElementBinary(eid, str, strlen(str)); }

			void Complete() throw();

		protected:
			void InitMaster(uint32_t id, uint64_t maybe_size = 0) throw();

		private:
			Core::IOCallback* _callback;
			int64_t _cur_pos;
			int32_t _size_offset_bytes;
		};
	}
}

#endif //__AKA_MATROSKA_EBML_H