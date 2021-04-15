#include <time.h>
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "avfilter.h"
#include "formats.h"
#include "internal.h"
#include "video.h"

#include "qndeblock-sdk.h"


typedef struct QNDeblockContext {
	const AVClass* class;

	const char* model_url;
	QNDeblockHandle qndeblockHandle;
	unsigned char* i420buffer;

} QNDeblockContext;

typedef struct ThreadData {
	AVFrame* in, * out;

} ThreadData;

static int avframe_2_I420buffer(AVFrame* in, unsigned char* buffer)
{
	unsigned char* y = buffer;
	unsigned char* u = buffer + in->width * in->height;
	unsigned char* v = buffer + in->width * in->height * 5 / 4;
	int i;
	for (i = 0; i < in->height; i++)
		memcpy(y + i * in->width, in->data[0] + in->linesize[0] * i, in->width);

	for (i = 0; i < in->height / 2; i++)
	{
		memcpy(u + i * in->width / 2, in->data[1] + in->linesize[1] * i, in->width / 2);
		memcpy(v + i * in->width / 2, in->data[2] + in->linesize[2] * i, in->width / 2);
	}

	return 0;
}

static int I420buffer_2_avframe(unsigned char* buffer, AVFrame* out)
{
	unsigned char* y = buffer;
	unsigned char* u = buffer + out->width * out->height;
	unsigned char* v = buffer + out->width * out->height * 5 / 4;
	int i;
	for (i = 0; i < out->height; i++)
		memcpy(out->data[0] + out->linesize[0] * i, y + i * out->width, out->width);

	for (i = 0; i < out->height / 2; i++)
	{
		memcpy(out->data[1] + out->linesize[1] * i, u + i * out->width / 2, out->width / 2);
		memcpy(out->data[2] + out->linesize[2] * i, v + i * out->width / 2, out->width / 2);
	}

	return 0;
}

// core function
static int do_filter(AVFilterContext* ctx, void* arg, int jobnr, int nb_jobs)
{
	//	av_log(NULL, AV_LOG_WARNING, "### Leon's QN filter_frame: core function \n");
	if (jobnr != 0)
		return 0;

	QNDeblockContext* qnCtx = ctx->priv;
	ThreadData* td = arg;
	AVFrame* dst = td->out;
	AVFrame* src = td->in;

	//clock_t t1 = clock();
	avframe_2_I420buffer(src, qnCtx->i420buffer);
	QNDeblockFilter_Process_I420(qnCtx->qndeblockHandle, qnCtx->i420buffer);
	I420buffer_2_avframe(qnCtx->i420buffer, dst);
	//clock_t t2 = clock();
	//int tt = t2 - t1;
	//av_log(NULL, AV_LOG_INFO, "QNFILTER time : %d, %d %d\n", tt, jobnr, nb_jobs);

	return 0;
}

static int filter_frame(AVFilterLink* link, AVFrame* in)
{
	//av_log(NULL, AV_LOG_WARNING, "### Leon's QN filter_frame, link %x, frame %x \n", link, in);
	AVFilterContext* avctx = link->dst;
	AVFilterLink* outlink = avctx->outputs[0];
	AVFrame* out;
	const int nb_threads = ff_filter_get_nb_threads(avctx);

	//allocate a new buffer, data is null
	out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
	if (!out) {
		av_frame_free(&in);
		return AVERROR(ENOMEM);
	}

	// copy frame prop
	av_frame_copy_props(out, in);
	out->width = outlink->w;
	out->height = outlink->h;

	ThreadData td;
	td.in = in;
	td.out = out;
	int res;
	if (res = avctx->internal->execute(avctx, do_filter, &td, NULL, FFMIN(outlink->h, nb_threads))) {
		return res;
	}

	av_frame_free(&in);

	return ff_filter_frame(outlink, out);
}

static av_cold int config_output(AVFilterLink* outlink)
{
	AVFilterContext* ctx = outlink->src;
	QNDeblockContext* qnCtx = ctx->priv;

	//you can modify output width/height here
	outlink->w = ctx->inputs[0]->w;
	outlink->h = ctx->inputs[0]->h;
	av_log(NULL, AV_LOG_DEBUG, "configure output, w h = (%d %d), format %d \n", outlink->w, outlink->h, outlink->format);

	if (strcmp(qnCtx->model_url, "")==0)
	{
		av_log(qnCtx, AV_LOG_ERROR, "modelurl error [%s] \n", qnCtx->model_url);
		return AVERROR(EINVAL);
	}

	qnCtx->qndeblockHandle = NULL;
	if (QNDeblockFilter_Create(&qnCtx->qndeblockHandle, outlink->w, outlink->h, qnCtx->model_url) != 0) {
		av_log(qnCtx, AV_LOG_ERROR, "QNDeblockFilter_Create fail\n");
		return AVERROR(EINVAL);
	}

	qnCtx->i420buffer = (unsigned char*)malloc(outlink->w * outlink->h * 3 / 2);
	if (qnCtx->i420buffer == NULL) {
		av_log(qnCtx, AV_LOG_ERROR, "malloc i420buffer fail\n");
		return AVERROR(EINVAL);
	}

	return 0;
}

static av_cold int init(AVFilterContext* ctx)
{
	av_log(NULL, AV_LOG_DEBUG, "init \n");
	QNDeblockContext* privCtx = ctx->priv;
	//init something here if you want
	return 0;
}

static av_cold void uninit(AVFilterContext* ctx)
{
	av_log(NULL, AV_LOG_DEBUG, "uninit \n");
	QNDeblockContext* qnCtx = ctx->priv;
	//uninit something here if you want

	free(qnCtx->i420buffer);
	qnCtx->i420buffer = NULL;
	QNDeblockFilter_Destroy(qnCtx->qndeblockHandle);
	qnCtx->qndeblockHandle = NULL;
}

//currently we just support the most common YUV420, can add more if needed
static int query_formats(AVFilterContext* ctx)
{
	static const enum AVPixelFormat pix_fmts[] = {
		AV_PIX_FMT_YUV420P,
		AV_PIX_FMT_NONE
	};
	AVFilterFormats* fmts_list = ff_make_format_list(pix_fmts);
	if (!fmts_list)
		return AVERROR(ENOMEM);
	return ff_set_common_formats(ctx, fmts_list);
}

//*************
#define OFFSET(x) offsetof(QNDeblockContext, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM|AV_OPT_FLAG_FILTERING_PARAM

static const AVOption qndeblock_options[] = {
	{ "modelurl",         "model filename: [d:/nir10_best.onnx]",          OFFSET(model_url), AV_OPT_TYPE_STRING, {.str = ""}, .flags = VE},
	{ NULL }

};// TODO: add something if needed

static const AVClass qndeblock_class = {
	.class_name = "qndeblock",
	.item_name = av_default_item_name,
	.option = qndeblock_options,
	.version = LIBAVUTIL_VERSION_INT,
	.category = AV_CLASS_CATEGORY_FILTER,
};

static const AVFilterPad avfilter_vf_qndeblock_inputs[] = {
	{
		.name = "qndeblock_inputpad",
		.type = AVMEDIA_TYPE_VIDEO,
		.filter_frame = filter_frame,
	},
	{ NULL }
};

static const AVFilterPad avfilter_vf_qndeblock_outputs[] = {
	{
		.name = "qndeblock_outputpad",
		.type = AVMEDIA_TYPE_VIDEO,
		.config_props = config_output,
	},
	{ NULL }
};

AVFilter ff_vf_qndeblock = {
	.name = "qndeblock",
	.description = NULL_IF_CONFIG_SMALL("qiniu deblock filter"),
	.priv_size = sizeof(QNDeblockContext),
	.priv_class = &qndeblock_class,
	.init = init,
	.uninit = uninit,
	.query_formats = query_formats,
	.inputs = avfilter_vf_qndeblock_inputs,
	.outputs = avfilter_vf_qndeblock_outputs,
	.flags = AVFILTER_FLAG_SLICE_THREADS | AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
};

