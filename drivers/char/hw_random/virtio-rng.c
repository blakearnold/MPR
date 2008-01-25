/*
 * Randomness driver for virtio
 *  Copyright (C) 2007 Rusty Russell IBM Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
#include <linux/err.h>
#include <linux/hw_random.h>
#include <linux/scatterlist.h>
#include <linux/spinlock.h>
#include <linux/virtio.h>
#include <linux/virtio_rng.h>

static struct virtqueue *vq;
static u32 random_data;
static bool have_data;

static bool random_recv_done(struct virtqueue *vq)
{
	have_data = true;

	/* Don't suppress callbacks: there can't be any more since we
	 * have used up the only buffer. */
	return true;
}

static void register_buffer(void)
{
	struct scatterlist sg;

	sg_init_one(&sg, &random_data, sizeof(random_data));
	/* There should always be room for one buffer. */
	if (vq->vq_ops->add_buf(vq, &sg, 0, 1, &random_data) != 0)
		BUG();
	vq->vq_ops->kick(vq);
}

static int virtio_data_present(struct hwrng *rng)
{
	return have_data;
}

static int virtio_data_read(struct hwrng *rng, u32 *data)
{
	BUG_ON(!have_data);
	*data = random_data;

	have_data = false;
	register_buffer();
	return sizeof(*data);
}

static struct hwrng virtio_hwrng = {
	.name = "virtio",
	.data_present = virtio_data_present,
	.data_read = virtio_data_read,
};

static int virtrng_probe(struct virtio_device *vdev)
{
	int err;

	/* We expect a single virtqueue. */
	vq = vdev->config->find_vq(vdev, 0, random_recv_done);
	if (IS_ERR(vq))
		return PTR_ERR(vq);

	err = hwrng_register(&virtio_hwrng);
	if (err) {
		vdev->config->del_vq(vq);
		return err;
	}

	register_buffer();
	return 0;
}

static void virtrng_remove(struct virtio_device *vdev)
{
	hwrng_unregister(&virtio_hwrng);
	vq->vq_ops->shutdown(vq);
	vdev->config->del_vq(vq);
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_RNG, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_rng = {
	.driver.name =	KBUILD_MODNAME,
	.driver.owner =	THIS_MODULE,
	.id_table =	id_table,
	.probe =	virtrng_probe,
	.remove =	__devexit_p(virtrng_remove),
};

static int __init init(void)
{
	return register_virtio_driver(&virtio_rng);
}

static void __exit fini(void)
{
	unregister_virtio_driver(&virtio_rng);
}
module_init(init);
module_exit(fini);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio random number driver");
MODULE_LICENSE("GPL");
