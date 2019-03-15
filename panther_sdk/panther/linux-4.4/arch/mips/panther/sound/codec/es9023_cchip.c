#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <linux/of_gpio.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/asound.h>


//#include <asm/mach-cheetah/cheetah.h>
//#include <asm/mach-cheetah/common.h>

#include "es9023_cchip.h"
#include <asm/dma.h>
//#include <asm/mach-cheetah/cheetah.h>
#include <asm/atomic.h>

//#include "../cta-pcm.h"




#define ES9023_DEBUG

#ifdef ES9023_DEBUG
#undef pr_debug
#define pr_debug(fmt, ...) \
    printk(KERN_EMERG pr_fmt(fmt), ##__VA_ARGS__)
#endif


int es9023_adcin= 0; // o:"Stereo", 1:"Mono (Left)", 2:"Mono (Right)"

int send_ble = 1;

extern  void uartd_write(const char *src, unsigned count);


#ifdef CONFIG_ES9023CONTROL
#define hotplug_path	uevent_helper
struct cta_vol_event{
	bool type;						   // 0:left 1:right
    char value[4];                    // "000"-"100"
    struct delayed_work gpioctrl;
};

static void cta_vol_event_hook(struct work_struct *work)
{
    struct cta_vol_event *pcm_event = (struct cta_vol_event *)container_of(work, struct cta_vol_event, gpioctrl.work);

	char *envp[3];
    char *argv[3] = {
        hotplug_path,
        "vol",
        NULL
    };

	envp[0] = kmalloc(sizeof("VALUE=")+ARRAY_SIZE(pcm_event->value) , GFP_ATOMIC);
	sprintf(envp[0],"VALUE=%s",pcm_event->value);
	
	envp[1] = kmalloc(sizeof("TYPE=") + 5, GFP_ATOMIC);
	sprintf(envp[1],"TYPE=%s",pcm_event->type ? "left" : "right");

	envp[2] = NULL;
	
    call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
   // kfree(env);
	kfree(envp[0]);
	kfree(envp[1]);
}

#endif

#define es9023_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 |\
	SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_32000 |\
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)
#define es9023_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE  | SNDRV_PCM_FMTBIT_S16_BE  | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE  | SNDRV_PCM_FMTBIT_S24_BE)

struct es9023_regs{
		unsigned int   left_volume ;
		unsigned int   right_volume ;
		unsigned int   enumerated;
		int   volume_min;
		int   volume_max;	
};
/* codec private data */
struct es9023_priv {
	struct snd_soc_codec *codec;
	struct es9023_platform_data *pdata;
	struct es9023_regs reg;
#ifdef CONFIG_ES9023CONTROL
	struct cta_vol_event pcm_event;
#endif
	//enum snd_soc_control_type control_type;
	void *control_data;
	unsigned mclk;
};



static int es9023_snd_soc_info_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	//snd_soc_component_get_drvdata(struct snd_soc_component * c)
	struct es9023_priv *es9023 =(struct es9023_priv *) snd_soc_component_get_drvdata(component);
	int platform_max;

	if (!mc->platform_max)
		mc->platform_max = mc->max;
	platform_max = mc->platform_max;

	if (platform_max == 1 && !strstr(kcontrol->id.name, " Volume"))
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;

	uinfo->count = snd_soc_volsw_is_stereo(mc) ? 2 : 1;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = platform_max;

	es9023->reg.volume_max = platform_max;
	es9023->reg.volume_min = 155;

	return 0;
}


static int es9023_snd_soc_write(struct es9023_priv *prtd, unsigned int volume, bool type) 
{
		char buf[32] = {0};
		char vol[4] = {0};
		memcpy(buf,"AXX+VOL+",strlen("AXX+VOL+"));
		pr_debug("%s:%d %d \n",__func__,__LINE__,volume);
		volume = volume - 155;	
		pr_debug("%s:%d after %d \n",__func__,__LINE__,volume);

		snprintf(vol, 4, "%03d",volume);
		pr_debug("%s:%d vol=%s \n",__func__,__LINE__,vol);
		//sshw_cdb_set("cver",vol);
			
		//snprintf(buf+sizeof("AXX+VOL+")-1, 4, "%03d&", volume);			
		strcat(buf,vol);
		strcat(buf,"&");
		printk(KERN_EMERG"%s:%d :uartd_write %s\n",__func__,__LINE__,buf);
		uartd_write(buf,strlen(buf));
		
#ifdef CONFIG_ES9023CONTROL
		prtd->pcm_event.type  = type; 
		memcpy(prtd->pcm_event.value, vol, 4);
       	schedule_delayed_work(&prtd->pcm_event.gpioctrl, 1); 
#endif
		
		return 0;
}

static int es9023_snd_soc_update_bits(struct snd_soc_component *codec,  bool type_2r,
				unsigned int mask, unsigned int value)
{
	bool change;
	unsigned int old, new;

	struct es9023_priv *es9023 = snd_soc_component_get_drvdata(codec);
	if(type_2r)
		old = es9023->reg.right_volume;
	else 
		old = es9023->reg.left_volume;
		
	new = (old & ~mask) | (value & mask);
	
	change = old != new;
	if(new > es9023->reg.volume_max){
		new = es9023->reg.volume_max;
	} 

	if(new < es9023->reg.volume_min) {
		new = es9023->reg.volume_min;
	}

	if (change) {
		if (es9023->reg.left_volume == es9023->reg.volume_min) {
			uartd_write("AXX+MUT+000&", sizeof("AXX+MUT+000&"));
		}
		if(type_2r) 
			es9023->reg.right_volume = new ;
		else 
			es9023->reg.left_volume = new ;
		//printk(KERN_EMERG"%s:%d new=%d \n",__func__,__LINE__ ,new);
		//es9023_snd_soc_write(es9023, new,type_2r);
	}
	
	return change;
}


static int es9023_snd_soc_update_bits_locked(struct snd_soc_component *codec,bool type_2r, 
	unsigned int mask, unsigned int value)
{
	int change;
	struct es9023_priv *es9023 = NULL;
	es9023 = snd_soc_component_get_drvdata(codec);
	change = es9023_snd_soc_update_bits(codec, type_2r, mask, value);

	return change;
}


static int es9023_snd_soc_get_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	//struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct es9023_priv *es9023 = (struct es9023_priv *)snd_soc_component_get_drvdata(component);
	
	unsigned int reg = mc->reg; 
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	
	ucontrol->value.integer.value[0] =
		(es9023->reg.left_volume >> shift) & mask;
	if (invert)
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
	
	if (snd_soc_volsw_is_stereo(mc)) {
		if (reg == reg2)
			ucontrol->value.integer.value[1] =
				(es9023->reg.right_volume >> rshift) & mask;
		else
			ucontrol->value.integer.value[1] =
				(es9023->reg.right_volume>> shift) & mask;
		if (invert)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
	}
	pr_debug("%s:%d val[0]=%ld,val[1]=%ld\n",__func__,__LINE__ ,ucontrol->value.integer.value[0] ,ucontrol->value.integer.value[1]);
	return 0;
}



static int es9023_snd_soc_put_volsw(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct es9023_priv *es9023 = (struct es9023_priv *)snd_soc_component_get_drvdata(component);

	unsigned int reg = mc->reg; 
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	int min = mc->min;
	unsigned int mask = (1 << fls(max)) - 1;
	unsigned int invert = mc->invert;
	int err;
	bool type_2r = 0;
	unsigned int val2 = 0;
	unsigned int val, val_mask;
	
	val = (ucontrol->value.integer.value[0] & mask);
	if (invert)
		val = max - val;
	val_mask = mask << shift;
	val = val << shift;
	
	if (snd_soc_volsw_is_stereo(mc)) {
		val2 = (ucontrol->value.integer.value[1] & mask);
		if (invert)
			val2 = max - val2;
		if (reg == reg2) {
			val_mask |= mask << rshift;
			val |= val2 << rshift;
		} else {
			val2 = val2 << shift;
			type_2r = 1;
		}
	}
	if(val >= max)
		val = max;
	
	if(val < min)
			val = min;

	if(val <= 100) {
		val = 155 + val;
		val2 = val;
		send_ble = 0;
	} else {
		send_ble = 1;
	}

	err = es9023_snd_soc_update_bits_locked(component, 0, val_mask, val);
	if (err < 0) {
		return err;
	}
	
	if (type_2r)  {
		if(val2 >= max)
			val2 = max;
		err = es9023_snd_soc_update_bits_locked(component, 1, val_mask, val2);
	}
	pr_debug("%s:%d val=%d val2=%d\n",__func__,__LINE__ ,val, val2);
	

	if(err && send_ble) {
		es9023_snd_soc_write(es9023, val,type_2r);
		
	}
	return err;
}
	
static int es9023_snd_soc_info_adcin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_info *uinfo)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	int channels = e->shift_l == e->shift_r ? 1 : 2;
	unsigned int items = e->items;
	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = channels;
	uinfo->value.enumerated.items = items;
	if (!items) {
		return 0;
	}
	if (uinfo->value.enumerated.item >= items)
		uinfo->value.enumerated.item = items - 1;
	WARN(strlen(e->texts[uinfo->value.enumerated.item]) >= sizeof(uinfo->value.enumerated.name),
		 "ALSA: too long item name '%s'\n",
		 e->texts[uinfo->value.enumerated.item]);
	strlcpy(uinfo->value.enumerated.name,
		e->texts[uinfo->value.enumerated.item],
		sizeof(uinfo->value.enumerated.name));
	return 0;
}

	

static int es9023_snd_soc_get_adcin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int val, item;
	unsigned int reg_val;
	
	//ret = snd_soc_component_read(component, e->reg, &reg_val);
	//if (ret)
	//	return ret;
	reg_val = es9023_adcin;
	
	val = (reg_val >> e->shift_l) & e->mask;
	
	item = snd_soc_enum_val_to_item(e, val);
	ucontrol->value.enumerated.item[0] = item;
	if (e->shift_l != e->shift_r) {
		val = (reg_val >> e->shift_l) & e->mask;
		item = snd_soc_enum_val_to_item(e, val);
		ucontrol->value.enumerated.item[1] = item;
	}

	return 0;
}

	
static  int es9023_snd_soc_put_adcin(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;
	unsigned int val;
	unsigned int mask;
	if (item[0] >= e->items) {
		return -EINVAL;
	}
	val = snd_soc_enum_item_to_val(e, item[0]) << e->shift_l;
	mask = e->mask << e->shift_l;
	if (e->shift_l != e->shift_r) {
		if (item[1] >= e->items) {
			return -EINVAL;
		}
		val |= snd_soc_enum_item_to_val(e, item[1]) << e->shift_r;
		mask |= e->mask << e->shift_r;
	}
	es9023_adcin = val;
	//return snd_soc_component_update_bits(component, e->reg, mask, val);
	return 0;
}



/*
 * ES9023 Controls
 */

static const char *es9023_mono_mux[] = {"Stereo", "Mono (Left)",
	"Mono (Right)"};



static const struct soc_enum es9023_enum[] = {
SOC_ENUM_SINGLE(ES9023_ADCIN, 6, 3, es9023_mono_mux), /* 16 */
};


static const struct snd_kcontrol_new es9023_snd_controls[] = {
ES9023_SOC_DOUBLE_R("PCM Volume", ES9023_LDAC, ES9023_RDAC, 0, 255, 0),
ES9023_SOC_ENUM("Mixer Playback Switch", es9023_enum[0]),

};




static int es9023_set_dai_sysclk(struct snd_soc_dai *codec_dai,
		int clk_id, unsigned int freq, int dir)
{
		
	struct snd_soc_codec *codec = codec_dai->codec;
	struct es9023_priv *es9023 = snd_soc_codec_get_drvdata(codec);
	switch (freq) {
	case 11289600:
	case 12000000:
	case 12288000:
	case 16934400:
	case 18432000:
		es9023->mclk = freq;
		return 0;
	}
	return 0;
}

static int es9023_set_dai_fmt(struct snd_soc_dai *codec_dai,
				  unsigned int fmt)
{

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		break;
	default:
		return -EINVAL;
	}
	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	case SND_SOC_DAIFMT_IB_NF:
		break;
	case SND_SOC_DAIFMT_IB_IF:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int es9023_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	unsigned int rate;
	
	rate = params_rate(params);
	pr_debug("rate: %u\n", rate);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
		pr_debug("24bit\n");
		/* fall through */
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S20_3BE:
		pr_debug("20bit\n");
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		pr_debug("16bit\n");

		break;
	default:
		return -EINVAL;
	}
	return 0;
}
#if 0
static int es9023_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	pr_debug("%s,level = %d\n",__func__,level);
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* Full power on */

		break;

	case SND_SOC_BIAS_STANDBY:
		if (codec->dapm.bias_level == SND_SOC_BIAS_OFF) {
		}

		/* Power up to mute */
		/* FIXME */
		break;

	case SND_SOC_BIAS_OFF:
		/* The chip runs through the power down sequence for us. */
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}
#endif

static int es9023_mute(struct snd_soc_dai *dai, int mute)
{
	struct snd_soc_codec *codec = NULL; 
	struct es9023_priv *es9023  = NULL; 
	codec = dai->codec;
	es9023 = snd_soc_codec_get_drvdata(codec);
	/*
	if(mute) {
			uartd_write("AXX+MUT+001&", strlen("AXX+MUT+001&"));
			//printk(KERN_EMERG"%s:%d mute on\n",__func__,__LINE__ );
			pr_debug("%s:%d mute on\n",__func__,__LINE__ );
	} else {
			pr_debug("%s:%d mute off\n",__func__,__LINE__ );
			if(es9023->reg.left_volume != es9023->reg.volume_min) {
				//printk(KERN_EMERG"%s:%d mute off\n",__func__,__LINE__ );
				uartd_write("AXX+MUT+000&", strlen("AXX+MUT+000&"));
			}
		
	}
	*/
	return 0;
		
}


static const struct snd_soc_dai_ops es9023_dai_ops = {
	.hw_params			= es9023_hw_params,
	.set_sysclk			= es9023_set_dai_sysclk,
	.set_fmt			= es9023_set_dai_fmt,
	.digital_mute 		= es9023_mute,
	


};

static struct snd_soc_dai_driver es9023_dai_driver = {
	.name = "es9023",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = es9023_RATES,
		.formats = es9023_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = es9023_RATES,
		.formats = es9023_FORMATS,
	},
	.ops = &es9023_dai_ops,
};

static int es9023_probe(struct snd_soc_codec *codec)
{
 
	printk(KERN_EMERG"%s:%d : es9023_probe\n",__func__,__LINE__);

	return 0;
}

static int es9023_remove(struct snd_soc_codec *codec)
{
	return 0;
}


static int es9023_suspend(struct snd_soc_codec *codec)
{

	
	return 0;
}

static int es9023_resume(struct snd_soc_codec *codec)
{

	return 0;
}

static const struct snd_soc_codec_driver soc_codec_dev_es9023 = {
	.probe =		es9023_probe,
	.remove =		es9023_remove,
	.suspend =		es9023_suspend,
	.resume =		es9023_resume,


	.controls = es9023_snd_controls,
	.num_controls = ARRAY_SIZE(es9023_snd_controls),

};

static     int es9023_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	
	struct es9023_priv *es9023;
	int ret;
	
	printk(KERN_EMERG"%s:%d : es9023_i2c_probe\n",__func__,__LINE__);
	es9023 = devm_kzalloc(&i2c->dev, sizeof(struct es9023_priv),
			      GFP_KERNEL);
	if (!es9023)
		return -ENOMEM;
	
	es9023->reg.volume_max = 255;
	es9023->reg.volume_min = 155;
#ifdef CONFIG_ES9023CONTROL
	INIT_DELAYED_WORK(&es9023->pcm_event.gpioctrl, cta_vol_event_hook);
#endif
	i2c_set_clientdata(i2c, es9023);
	//es9023->control_type = SND_SOC_I2C;
	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_es9023, 
			&es9023_dai_driver, 1);
	printk(KERN_EMERG"%s:%d :snd_soc_register_codec ret:%d\n",__func__,__LINE__, ret);
	if (ret != 0){
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);
	}
	return ret;
}

static  int es9023_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);
	devm_kfree(&client->dev, i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id es9023_i2c_id[] = {
	{ "es9023", 0 },
	{ }
};

static struct i2c_driver es9023_i2c_driver = {
	.driver = {
		.name = "es9023",
		.owner = THIS_MODULE,
	},
	.probe =    es9023_i2c_probe,
	.remove =   es9023_i2c_remove,
	.id_table = es9023_i2c_id,
};



static int __init ES9023_init(void)
{

	return i2c_add_driver(&es9023_i2c_driver);
}

static void __exit ES9023_exit(void)
{
	i2c_del_driver(&es9023_i2c_driver);
}
module_init(ES9023_init);
module_exit(ES9023_exit);
