#ifndef SFX_PREVIEW_GENERATOR_H
#define SFX_PREVIEW_GENERATOR_H

#include "scene/main/node.h"
#include "servers/audio/audio_stream.h"

class SFXPreview;

class _SFXPreviewGenerator : public Node {
	GDCLASS(_SFXPreviewGenerator, Node);

	static _SFXPreviewGenerator* singleton;

	struct Preview {
		Ref<SFXPreview> preview;
		Ref<AudioStream> base_stream;
		Ref<AudioStreamPlayback> playback;
		SafeFlag generating;
		ObjectID id;
		Thread* thread = nullptr;

		// Needed for the bookkeeping of the Map
		void operator=(const Preview& p_rhs) {
			preview = p_rhs.preview;
			base_stream = p_rhs.base_stream;
			playback = p_rhs.playback;
			generating.set_to(generating.is_set());
			id = p_rhs.id;
			thread = p_rhs.thread;
		}
		Preview(const Preview& p_rhs) {
			preview = p_rhs.preview;
			base_stream = p_rhs.base_stream;
			playback = p_rhs.playback;
			generating.set_to(generating.is_set());
			id = p_rhs.id;
			thread = p_rhs.thread;
		}
		Preview() {}
	};

	HashMap<ObjectID, Preview> previews;

	static void _preview_thread(void* p_preview);

	void _update_emit(ObjectID p_id);

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	static _SFXPreviewGenerator* get_singleton() {
		if (singleton == nullptr) {
			singleton = memnew(_SFXPreviewGenerator);
		}
		return singleton;
	}

	Ref<SFXPreview> generate_preview(const Ref<AudioStream>& p_stream);

	_SFXPreviewGenerator();
};

#endif
