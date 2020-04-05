#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/slab.h>

enum mcp_type {
	mcp_23008,
	mcp_23017,
};

struct chip_desc {
	bool double_port;
};

struct mcpdmg {
	char *name;
	const struct chip_desc *desc;
	unsigned controller_port;

	struct gpio_chip *chip;

	struct i2c_client *client;

	struct mutex lock;
};

static const struct chip_desc chips[] = {
    [mcp_23008] = {
        .double_port = false,
    },
    [mcp_23017] = {
        .double_port = true,
    },
};

static const struct i2c_device_id mcp23xxx_id[] = { { "mcp23008", mcp_23008 },
						    { "mcp23017", mcp_23017 },
						    {} };

MODULE_DEVICE_TABLE(i2c, mcp23xxx_id);

static int mcpdmg_dt_parse(struct i2c_client *client, struct mcpdmg *chip)
{
	struct device_node *dt_node = client->dev.of_node;
	const char *ic_name;

	if (of_property_read_string(dt_node, "ic-name", &ic_name))
		return -1;
	chip->name = (unsigned char *)ic_name;

	return 0;
}

#define MCPDMG_OUTPUT 0
#define MCPDMG_INPUT 1

static int mcpdmg_get_direction(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static int mcpdmg_direction(struct gpio_chip *gc, unsigned offset,
			    unsigned direction, int val)
{
	return 0;
}

static int mcpdmg_direction_output(struct gpio_chip *gc, unsigned offset,
				   int val)
{
	return mcpdmg_direction(gc, offset, MCPDMG_OUTPUT, val);
}

static int mcpdmg_direction_input(struct gpio_chip *gc, unsigned offset)
{
	return mcpdmg_direction(gc, offset, MCPDMG_INPUT, 0);
}

static int mcpdmg_get_value(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static int mcpdmg_get_multiple(struct gpio_chip *chip, unsigned long *mask,
			       unsigned long *bits)
{
	return 0;
}

static void mcpdmg_set_value(struct gpio_chip *chip, unsigned offset, int value)
{
}

static void mcpdmg_set_multiple(struct gpio_chip *chip, unsigned long *mask,
				unsigned long *bits)
{
}

// Setup port used for game controller
static int mcpdmg_init_dmg_port(struct mcpdmg *mc)
{
	return 0;
}

static int mcpdmg_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct mcpdmg *data =
		devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	//TODO - check functionality?

	mutex_init(&data->lock);

	data->client = client;

	if (client->dev.of_node)
		if (mcpdmg_dt_parse(client, data) < 0)
			goto out;

	// If 16 pin MCP, 8 extra GPIO to use freely.
	if (data->desc->double_port) {
		data->chip = devm_kzalloc(&client->dev, sizeof(*data->chip),
					  GFP_KERNEL);
		if (!data->chip)
			return -ENOMEM;

		data->chip->label = client->name;
		data->chip->base = -1;
		data->chip->owner = THIS_MODULE;
		data->chip->ngpio = 8;
		data->chip->get = mcpdmg_get_value;
		data->chip->set = mcpdmg_set_value;
		data->chip->set_multiple = mcpdmg_set_multiple;
		data->chip->direction_input = mcpdmg_direction_input;
		data->chip->direction_output = mcpdmg_direction_output;
		data->chip->get_direction = mcpdmg_get_direction;

		if (gpiochip_add(data->chip))
			goto out;
	}

	i2c_set_clientdata(client, data);

	if (mcpdmg_init_dmg_port(data) < 0)
		goto out;

	return 0;

out:
	dev_err(&client->dev, "Failed to load dev %s", data->name);

	return -1;
}

static int mcpdmg_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver mcpdmg_driver = {
    .driver = {
        .name = "mcpdmg",
        /** .pm = &mcpdmg_pm, */
    },
    .probe = mcpdmg_probe,
    .remove = mcpdmg_remove,
    .id_table = mcp23xxx_id,
};

static int __init mcpdmg_init(void)
{
	return i2c_add_driver(&mcpdmg_driver);
}

static void __exit mcpdmg_cleanup(void)
{
	i2c_del_driver(&mcpdmg_driver);
}

module_init(mcpdmg_init);
module_exit(mcpdmg_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alex Hoffman");

