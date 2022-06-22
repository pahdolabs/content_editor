#include "sfx_preview_generator.h"

#include "sfx_preview.h"
#include "servers/audio_server.h"

void _SFXPreviewGenerator::_update_emit(ObjectID p_id) {
	emit_signal("preview_updated", p_id);
}

void _SFXPreviewGenerator::_preview_thread(void* p_preview) {
	Preview* preview = static_cast<Preview*>(p_preview);

	float muxbuff_chunk_s = 0.25;

	int mixbuff_chunk_frames = AudioServer::get_singleton()->get_mix_rate() * muxbuff_chunk_s;

	Vector<AudioFrame> mix_chunk;
	mix_chunk.resize(mixbuff_chunk_frames);

	int frames_total = AudioServer::get_singleton()->get_mix_rate() * preview->preview->length;
	int frames_todo = frames_total;

	preview->playback->start();

	while (frames_todo) {
		int ofs_write = uint64_t(frames_total - frames_todo) * uint64_t(preview->preview->preview.size() / 2) / uint64_t(frames_total);
		int to_read = MIN(frames_todo, mixbuff_chunk_frames);
		int to_write = uint64_t(to_read) * uint64_t(preview->preview->preview.size() / 2) / uint64_t(frames_total);
		to_write = MIN(to_write, (preview->preview->preview.size() / 2) - ofs_write);

		preview->playback->mix(mix_chunk.ptrw(), 1.0, to_read);

		for (int i = 0; i < to_write; i++) {
			float max = -1000;
			float min = 1000;
			int from = uint64_t(i) * to_read / to_write;
			int to = (uint64_t(i) + 1) * to_read / to_write;
			to = MIN(to, to_read);
			from = MIN(from, to_read - 1);
			if (to == from) {
				to = from + 1;
			}

			for (int j = from; j < to; j++) {
				max = MAX(max, mix_chunk[j].l);
				max = MAX(max, mix_chunk[j].r);

				min = MIN(min, mix_chunk[j].l);
				min = MIN(min, mix_chunk[j].r);
			}

			uint8_t pfrom = CLAMP((min * 0.5 + 0.5) * 255, 0, 255);
			uint8_t pto = CLAMP((max * 0.5 + 0.5) * 255, 0, 255);

			preview->preview->preview.write[(ofs_write + i) * 2 + 0] = pfrom;
			preview->preview->preview.write[(ofs_write + i) * 2 + 1] = pto;
		}

		frames_todo -= to_read;
		singleton->call_deferred("_update_emit", preview->id);
	}

	preview->playback->stop();

	preview->generating.clear();
}

Ref<SFXPreview> _SFXPreviewGenerator::generate_preview(const Ref<AudioStream>& p_stream) {
	ERR_FAIL_COND_V(p_stream.is_null(), Ref<SFXPreview>());

	if (previews.has(p_stream->get_instance_id())) {
		return previews[p_stream->get_instance_id()].preview;
	}

	//no preview exists

	previews[p_stream->get_instance_id()] = Preview();

	Preview* preview = &previews[p_stream->get_instance_id()];
	preview->base_stream = p_stream;
	preview->playback = preview->base_stream->instance_playback();
	preview->generating.set();
	preview->id = p_stream->get_instance_id();

	float len_s = preview->base_stream->get_length();
	if (len_s == 0) {
		len_s = 60 * 5; //five minutes
	}

	int frames = AudioServer::get_singleton()->get_mix_rate() * len_s;

	Vector<uint8_t> maxmin;
	int pw = frames / 20;
	maxmin.resize(pw * 2);
	{
		uint8_t* ptr = maxmin.ptrw();
		for (int i = 0; i < pw * 2; i++) {
			ptr[i] = 127;
		}
	}

	preview->preview.instance();
	preview->preview->preview = maxmin;
	preview->preview->length = len_s;

	if (preview->playback.is_valid()) {
		preview->thread = memnew(Thread);
		preview->thread->start(_preview_thread, preview);
	}

	return preview->preview;
}

void _SFXPreviewGenerator::_bind_methods() {
	ClassDB::bind_method("_update_emit", &_SFXPreviewGenerator::_update_emit);
	ClassDB::bind_method(D_METHOD("generate_preview", "stream"), &_SFXPreviewGenerator::generate_preview);

	ADD_SIGNAL(MethodInfo("preview_updated", PropertyInfo(Variant::INT, "obj_id")));
}

_SFXPreviewGenerator* _SFXPreviewGenerator::singleton = nullptr;

void _SFXPreviewGenerator::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_PROCESS: {
		List<ObjectID> to_erase;
		const HashMap<ObjectID, Preview>::Pair *key_values;
		previews.get_key_value_ptr_array(&key_values);
		for (int i = 0; i < previews.size(); ++i) {
			HashMap<ObjectID, Preview>::Pair kv = key_values[i];
			if (!kv.data.generating.is_set()) {
				if (kv.data.thread) {
					kv.data.thread->wait_to_finish();
					memdelete(kv.data.thread);
					kv.data.thread = nullptr;
				}
				if (!ObjectDB::get_instance(kv.key)) { //no longer in use, get rid of preview
					to_erase.push_back(kv.key);
				}
			}
		}

		while (to_erase.front()) {
			previews.erase(to_erase.front()->get());
			to_erase.pop_front();
		}
	} break;
	}
}

_SFXPreviewGenerator::_SFXPreviewGenerator() {
	set_process(true);
}
