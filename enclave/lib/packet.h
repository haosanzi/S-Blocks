//wait for delete
//#include <click/ipaddress.hh>
//#include <click/glue.hh>
//#include <click/timestamp.hh>
//#include <click/atomic.hh>
#ifndef _ENCLAVE_PACKET_H
#define _ENCLAVE_PACKET_H


//begin
#include "../include/config.h"
#include <string.h>
#include <stddef.h>
#include "./ipaddress.h"
#include "./packet_anno.h"
#include "../net/ether.h"
#include "../net/icmp.h"
#include "../net/tcp.h"
#include "../net/udp.h"
#include "../net/ip.h"
#include "../net/ip6.h"
#include "../include/atomic.h" //for atomic_uint32_t _use_count
#include "../lib/timestamp.h"



//???class IP6Address;
class WritablePacket;
class Packet {
public:
    enum{
        default_headroom = 28,      ///< Default packet headroom() for
                    ///  Packet::make().  4-byte aligned.
        min_buffer_length = 64      ///< Minimum buffer_length() for
                    ///  Packet::make()
    };

    
    static WritablePacket *make(uint32_t headroom, const void *data,
                uint32_t length, uint32_t tailroom) CLICK_WARN_UNUSED_RESULT;
    static inline WritablePacket *make(const void *data, uint32_t length) CLICK_WARN_UNUSED_RESULT;
    static inline WritablePacket *make(uint32_t length) CLICK_WARN_UNUSED_RESULT;
     typedef void (*buffer_destructor_type)(unsigned char* buf, size_t sz, void* argument);
    static WritablePacket* make(unsigned char* data, uint32_t length,
                buffer_destructor_type buffer_destructor,
                                void* argument = (void*) 0, int headroom = 0, int tailroom = 0) CLICK_WARN_UNUSED_RESULT;

    static void empty_destructor(unsigned char*, size_t, void*);

    static void static_cleanup();

    inline void kill();
    inline void kill_nonatomic();

    inline bool shared() const;
    Packet *clone(bool fast = false) CLICK_WARN_UNUSED_RESULT;
    inline WritablePacket *uniqueify() CLICK_WARN_UNUSED_RESULT;

    inline const unsigned char *data() const;
    inline const unsigned char *end_data() const;
    inline uint32_t length() const;
    inline uint32_t headroom() const;
    inline uint32_t tailroom() const;
    inline const unsigned char *buffer() const;
    inline const unsigned char *end_buffer() const;
    inline uint32_t buffer_length() const;

    buffer_destructor_type buffer_destructor() const {
    return _destructor;
    }
    void set_buffer_destructor(buffer_destructor_type destructor) {
        _destructor = destructor;
    }

    void* destructor_argument() {
        return _destructor_argument;
    }

    void reset_buffer() {
    assert(!shared());
    _head = _data = _tail = _end = 0;
    _destructor = 0;
    }

 /** @brief Add space for a header before the packet.
     * @param len amount of space to add
     * @return packet with added header space, or null on failure
     *
     * Returns a packet with an additional @a len bytes of uninitialized space
     * before the current packet's data().  A copy of the packet data is made
     * if there isn't enough headroom() in the current packet, or if the
     * current packet is shared().  If no copy is made, this operation is
     * quite efficient.
     *
     * If a data copy would be required, but the copy fails because of lack of
     * memory, then the current packet is freed.
     *
     * push() is usually used like this:
     * @code
     * WritablePacket *q = p->push(14);
     * if (!q)
     *     return 0;
     * // p must not be used here.
     * @endcode
     *
     * @post new length() == old length() + @a len (if no failure)
     *
     * @sa nonunique_push, push_mac_header, pull */
    WritablePacket *push(uint32_t len) CLICK_WARN_UNUSED_RESULT;

    /** @brief Add space for a MAC header before the packet.
     * @param len amount of space to add and length of MAC header
     * @return packet with added header space, or null on failure
     *
     * Combines the action of push() and set_mac_header().  @a len bytes are
     * pushed for a MAC header, and on success, the packet's returned MAC and
     * network header pointers are set as by set_mac_header(data(), @a len).
     *
     * @sa push */
    WritablePacket *push_mac_header(uint32_t len) CLICK_WARN_UNUSED_RESULT;

    /** @brief Add space for a header before the packet.
     * @param len amount of space to add
     * @return packet with added header space, or null on failure
     *
     * This is a variant of push().  Returns a packet with an additional @a
     * len bytes of uninitialized space before the current packet's data().  A
     * copy of the packet data is made if there isn't enough headroom() in the
     * current packet.  However, no copy is made if the current packet is
     * shared; and if no copy is made, this operation is quite efficient.
     *
     * If a data copy would be required, but the copy fails because of lack of
     * memory, then the current packet is freed.
     *
     * @note Unlike push(), nonunique_push() returns a Packet object, which
     * has non-writable data.
     *
     * @sa push */
    Packet *nonunique_push(uint32_t len) CLICK_WARN_UNUSED_RESULT;

    /** @brief Remove a header from the front of the packet.
     * @param len amount of space to remove
     *
     * Removes @a len bytes from the initial part of the packet, usually
     * corresponding to some network header (for example, pull(14) removes an
     * Ethernet header).  This operation is efficient: it just bumps a
     * pointer.
     *
     * It is an error to attempt to pull more than length() bytes.
     *
     * @post new data() == old data() + @a len
     * @post new length() == old length() - @a len
     *
     * @sa push */
    void pull(uint32_t len);

    /** @brief Add space for data after the packet.
     * @param len amount of space to add
     * @return packet with added trailer space, or null on failure
     *
     * Returns a packet with an additional @a len bytes of uninitialized space
     * after the current packet's data (starting at end_data()).  A copy of
     * the packet data is made if there isn't enough tailroom() in the current
     * packet, or if the current packet is shared().  If no copy is made, this
     * operation is quite efficient.
     *
     * If a data copy would be required, but the copy fails because of lack of
     * memory, then the current packet is freed.
     *
     * put() is usually used like this:
     * @code
     * WritablePacket *q = p->put(100);
     * if (!q)
     *     return 0;
     * // p must not be used here.
     * @endcode
     *
     * @post new length() == old length() + @a len (if no failure)
     *
     * @sa nonunique_put, take */
    WritablePacket *put(uint32_t len) CLICK_WARN_UNUSED_RESULT;

    /** @brief Add space for data after the packet.
     * @param len amount of space to add
     * @return packet with added trailer space, or null on failure
     *
     * This is a variant of put().  Returns a packet with an additional @a len
     * bytes of uninitialized space after the current packet's data (starting
     * at end_data()).  A copy of the packet data is made if there isn't
     * enough tailroom() in the current packet.  However, no copy is made if
     * the current packet is shared; and if no copy is made, this operation is
     * quite efficient.
     *
     * If a data copy would be required, but the copy fails because of lack of
     * memory, then the current packet is freed.
     *
     * @sa put */
    Packet *nonunique_put(uint32_t len) CLICK_WARN_UNUSED_RESULT;

    /** @brief Remove space from the end of the packet.
     * @param len amount of space to remove
     *
     * Removes @a len bytes from the end of the packet.  This operation is
     * efficient: it just bumps a pointer.
     *
     * It is an error to attempt to pull more than length() bytes.
     *
     * @post new data() == old data()
     * @post new end_data() == old end_data() - @a len
     * @post new length() == old length() - @a len
     *
     * @sa push */
    void take(uint32_t len);


    /** @brief Shift packet data within the data buffer.
     * @param offset amount to shift packet data
     * @param free_on_failure if true, then delete the input packet on failure
     * @return a packet with shifted data, or null on failure
     *
     * Useful to align packet data.  For example, if the packet's embedded IP
     * header is located at pointer value 0x8CCA03, then shift_data(1) or
     * shift_data(-3) will both align the header on a 4-byte boundary.
     *
     * If the packet is shared() or there isn't enough headroom or tailroom
     * for the operation, the packet is passed to uniqueify() first.  This can
     * fail if there isn't enough memory.  If it fails, shift_data returns
     * null, and if @a free_on_failure is true (the default), the input packet
     * is freed.
     *
     * The packet's mac_header, network_header, and transport_header areas are
     * preserved, even if they lie within the headroom.  Any headroom outside
     * these regions may be overwritten, as may any tailroom.
     *
     * @post new data() == old data() + @a offset (if no copy is made)
     * @post new buffer() == old buffer() (if no copy is made) */
    Packet *shift_data(int offset, bool free_on_failure = true) CLICK_WARN_UNUSED_RESULT;

    inline void shrink_data(const unsigned char *data, uint32_t length);
    inline void change_headroom_and_length(uint32_t headroom, uint32_t length);
    
    bool copy(Packet* p, int headroom=0);
    //@}

    /** @name Header Pointers */
    //@{
    inline bool has_mac_header() const;
    inline const unsigned char *mac_header() const;
    inline int mac_header_offset() const;
    inline uint32_t mac_header_length() const;
    inline int mac_length() const;
    inline void set_mac_header(const unsigned char *p);
    inline void set_mac_header(const unsigned char *p, uint32_t len);
    inline void clear_mac_header();

    inline bool has_network_header() const;
    inline const unsigned char *network_header() const;
    inline int network_header_offset() const;
    inline uint32_t network_header_length() const;
    inline int network_length() const;
    inline void set_network_header(const unsigned char *p, uint32_t len);
    inline void set_network_header_length(uint32_t len);
    inline void clear_network_header();

    inline bool has_transport_header() const;
    inline const unsigned char *transport_header() const;
    inline int transport_header_offset() const;
    inline int transport_length() const;
    inline void clear_transport_header();
    
    // CONVENIENCE HEADER ANNOTATIONS
    inline const click_ether *ether_header() const;
    inline void set_ether_header(const click_ether *ethh);

    inline const click_ip *ip_header() const;
    inline int ip_header_offset() const;
    inline uint32_t ip_header_length() const;
    inline void set_ip_header(const click_ip *iph, uint32_t len);

    inline const click_ip6 *ip6_header() const;
    inline int ip6_header_offset() const;
    inline uint32_t ip6_header_length() const;
    inline void set_ip6_header(const click_ip6 *ip6h);
    inline void set_ip6_header(const click_ip6 *ip6h, uint32_t len);

    inline const click_icmp *icmp_header() const;
    inline const click_tcp *tcp_header() const;
    inline const click_udp *udp_header() const;
    //@}
private:
    /** @cond never */
    union Anno;
    const Anno *xanno() const       { return &_aa.cb; }
    Anno *xanno()           { return &_aa.cb; }
    /** @endcond never */
  public:

    /** @name Annotations */
    //@{
    enum {
    anno_size = 48          ///< Size of annotation area.
    };

    /** @brief Return the timestamp annotation. */
    inline const Timestamp &timestamp_anno() const;
    /** @overload */
    inline Timestamp &timestamp_anno();
    /** @brief Set the timestamp annotation.
     * @param t new timestamp */
    inline void set_timestamp_anno(const Timestamp &t);

    /** @brief Return the device annotation. */
    //inline net_device *device_anno() const; //no element's function call this@zyh
    /** @brief Set the device annotation */
    //inline void set_device_anno(net_device *dev);//no element's function call this@zyh


    /** @brief Values for packet_type_anno().
     * Must agree with Linux's PACKET_ constants in <linux/if_packet.h>. */
    enum PacketType {
    HOST = 0,       /**< Packet was sent to this host. */
    BROADCAST = 1,      /**< Packet was sent to a link-level multicast
                     address. */
    MULTICAST = 2,      /**< Packet was sent to a link-level multicast
                     address. */
    OTHERHOST = 3,      /**< Packet was sent to a different host, but
                     received anyway.  The receiving device is
                     probably in promiscuous mode. */
    OUTGOING = 4,       /**< Packet was generated by this host and is
                     being sent elsewhere. */
    LOOPBACK = 5,
    FASTROUTE = 6
    };
    /** @brief Return the packet type annotation. */
    inline PacketType packet_type_anno() const;
    /** @brief Set the packet type annotation. */
    inline void set_packet_type_anno(PacketType t);

    /** @brief Return the next packet annotation. */
    inline Packet *next() const;
    /** @overload */
    inline Packet *&next();
    /** @brief Set the next packet annotation. */
    inline void set_next(Packet *p);

    /** @brief Return the previous packet annotation. */
    inline Packet *prev() const;
    /** @overload */
    inline Packet *&prev();
    /** @brief Set the previous packet annotation. */
    inline void set_prev(Packet *p);

     enum {
    dst_ip_anno_offset = 0, dst_ip_anno_size = 4,
    dst_ip6_anno_offset = 0, dst_ip6_anno_size = 16
    };

    /** @brief Return the destination IPv4 address annotation.
     *
     * The value is taken from the address annotation area. */
    inline IPAddress dst_ip_anno() const;

    /** @brief Set the destination IPv4 address annotation.
     *
     * The value is stored in the address annotation area. */
    inline void set_dst_ip_anno(IPAddress addr);

    /** @brief Return a pointer to the annotation area.
     *
     * The area is @link Packet::anno_size anno_size @endlink bytes long. */
    void *anno()            { return xanno(); }

    /** @overload */
    const void *anno() const        { return xanno(); }

    /** @brief Return a pointer to the annotation area as uint8_ts. */
    uint8_t *anno_u8()          { return &xanno()->u8[0]; }

    /** @brief overload */
    const uint8_t *anno_u8() const  { return &xanno()->u8[0]; }

    /** @brief Return a pointer to the annotation area as uint32_ts. */
    uint32_t *anno_u32()        { return &xanno()->u32[0]; }

    /** @brief overload */
    const uint32_t *anno_u32() const    { return &xanno()->u32[0]; }

    /** @brief Return annotation byte at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink */
    uint8_t anno_u8(int i) const {
    assert(i >= 0 && i < anno_size);
    return xanno()->u8[i];
    }

    /** @brief Set annotation byte at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink */
    void set_anno_u8(int i, uint8_t x) {
    assert(i >= 0 && i < anno_size);
    xanno()->u8[i] = x;
    }

    /** @brief Return 16-bit annotation at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 1
     * @pre On aligned targets, @a i must be evenly divisible by 2.
     *
     * Affects annotation bytes [@a i, @a i+1]. */
    uint16_t anno_u16(int i) const {
    assert(i >= 0 && i < anno_size - 1);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 2 == 0);
#endif
    return *reinterpret_cast<const click_aliasable_uint16_t *>(xanno()->c + i);
    }

    /** @brief Set 16-bit annotation at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 1
     * @pre On aligned targets, @a i must be evenly divisible by 2.
     *
     * Affects annotation bytes [@a i, @a i+1]. */
    void set_anno_u16(int i, uint16_t x) {
    assert(i >= 0 && i < anno_size - 1);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 2 == 0);
#endif
    *reinterpret_cast<click_aliasable_uint16_t *>(xanno()->c + i) = x;
    }

    /** @brief Return 16-bit annotation at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 1
     * @pre On aligned targets, @a i must be evenly divisible by 2.
     *
     * Affects annotation bytes [@a i, @a i+1]. */
    int16_t anno_s16(int i) const {
    assert(i >= 0 && i < anno_size - 1);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 2 == 0);
#endif
    return *reinterpret_cast<const click_aliasable_int16_t *>(xanno()->c + i);
    }

    /** @brief Set 16-bit annotation at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 1
     * @pre On aligned targets, @a i must be evenly divisible by 2.
     *
     * Affects annotation bytes [@a i, @a i+1]. */
    void set_anno_s16(int i, int16_t x) {
    assert(i >= 0 && i < anno_size - 1);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 2 == 0);
#endif
    *reinterpret_cast<click_aliasable_int16_t *>(xanno()->c + i) = x;
    }

    /** @brief Return 32-bit annotation at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 3
     * @pre On aligned targets, @a i must be evenly divisible by 4.
     *
     * Affects user annotation bytes [@a i, @a i+3]. */
    uint32_t anno_u32(int i) const {
    assert(i >= 0 && i < anno_size - 3);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 4 == 0);
#endif
    return *reinterpret_cast<const click_aliasable_uint32_t *>(xanno()->c + i);
    }

    /** @brief Set 32-bit annotation at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 3
     * @pre On aligned targets, @a i must be evenly divisible by 4.
     *
     * Affects user annotation bytes [@a i, @a i+3]. */
    void set_anno_u32(int i, uint32_t x) {
    assert(i >= 0 && i < anno_size - 3);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 4 == 0);
#endif
    *reinterpret_cast<click_aliasable_uint32_t *>(xanno()->c + i) = x;
    }

    /** @brief Return 32-bit annotation at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 3
     *
     * Affects user annotation bytes [4*@a i, 4*@a i+3]. */
    int32_t anno_s32(int i) const {
    assert(i >= 0 && i < anno_size - 3);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 4 == 0);
#endif
    return *reinterpret_cast<const click_aliasable_int32_t *>(xanno()->c + i);
    }

    /** @brief Set 32-bit annotation at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 3
     * @pre On aligned targets, @a i must be evenly divisible by 4.
     *
     * Affects user annotation bytes [@a i, @a i+3]. */
    void set_anno_s32(int i, int32_t x) {
    assert(i >= 0 && i < anno_size - 3);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % 4 == 0);
#endif
    *reinterpret_cast<click_aliasable_int32_t *>(xanno()->c + i) = x;
    }

#if HAVE_INT64_TYPES
    /** @brief Return 64-bit annotation at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 7
     * @pre On aligned targets, @a i must be aligned properly for uint64_t.
     *
     * Affects user annotation bytes [@a i, @a i+7]. */
    uint64_t anno_u64(int i) const {
    assert(i >= 0 && i < anno_size - 7);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % __alignof__(uint64_t) == 0);
#endif
    return *reinterpret_cast<const click_aliasable_uint64_t *>(xanno()->c + i);
    }

    /** @brief Set 64-bit annotation at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - 7
     * @pre On aligned targets, @a i must be aligned properly for uint64_t.
     *
     * Affects user annotation bytes [@a i, @a i+7]. */
    void set_anno_u64(int i, uint64_t x) {
    assert(i >= 0 && i < anno_size - 7);
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % __alignof__(uint64_t) == 0);
#endif
    *reinterpret_cast<click_aliasable_uint64_t *>(xanno()->c + i) = x;
    }
#endif

    /** @brief Return void * sized annotation at offset @a i.
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - sizeof(void *)
     * @pre On aligned targets, @a i must be aligned properly.
     *
     * Affects user annotation bytes [@a i, @a i+sizeof(void *)]. */
    void *anno_ptr(int i) const {
    assert(i >= 0 && i <= anno_size - (int)sizeof(void *));
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % __alignof__(void *) == 0);
#endif
    return *reinterpret_cast<const click_aliasable_void_pointer_t *>(xanno()->c + i);
    }

    /** @brief Set void * sized annotation at offset @a i.
     * @param i annotation offset in bytes
     * @param x value
     * @pre 0 <= @a i < @link Packet::anno_size anno_size @endlink - sizeof(void *)
     * @pre On aligned targets, @a i must be aligned properly.
     *
     * Affects user annotation bytes [@a i, @a i+sizeof(void *)]. */
    void set_anno_ptr(int i, const void *x) {
    assert(i >= 0 && i <= anno_size - (int)sizeof(void *));
#if !HAVE_INDIFFERENT_ALIGNMENT
    assert(i % __alignof__(void *) == 0);
#endif
    *reinterpret_cast<click_aliasable_void_pointer_t *>(xanno()->c + i) = const_cast<void *>(x);
    }



    inline void clear_annotations(bool all = true);
    inline void copy_annotations(const Packet *);
    //@}

    /** @cond never */
    enum {
    DEFAULT_HEADROOM = default_headroom,
    MIN_BUFFER_LENGTH = min_buffer_length,
    addr_anno_offset = 0,
    addr_anno_size = 16,
    user_anno_offset = 16,
    user_anno_size = 32,
    ADDR_ANNO_SIZE = addr_anno_size,
    USER_ANNO_SIZE = user_anno_size,
    USER_ANNO_U16_SIZE = USER_ANNO_SIZE / 2,
    USER_ANNO_U32_SIZE = USER_ANNO_SIZE / 4,
    USER_ANNO_U64_SIZE = USER_ANNO_SIZE / 8
    } CLICK_PACKET_DEPRECATED_ENUM;
    inline const unsigned char *buffer_data() const CLICK_DEPRECATED;
    inline void *addr_anno() CLICK_DEPRECATED;
    inline const void *addr_anno() const CLICK_DEPRECATED;
    inline void *user_anno() CLICK_DEPRECATED;
    inline const void *user_anno() const CLICK_DEPRECATED;
    inline uint8_t *user_anno_u8() CLICK_DEPRECATED;
    inline const uint8_t *user_anno_u8() const CLICK_DEPRECATED;
    inline uint32_t *user_anno_u32() CLICK_DEPRECATED;
    inline const uint32_t *user_anno_u32() const CLICK_DEPRECATED;
    inline uint8_t user_anno_u8(int i) const CLICK_DEPRECATED;
    inline void set_user_anno_u8(int i, uint8_t v) CLICK_DEPRECATED;
    inline uint16_t user_anno_u16(int i) const CLICK_DEPRECATED;
    inline void set_user_anno_u16(int i, uint16_t v) CLICK_DEPRECATED;
    inline uint32_t user_anno_u32(int i) const CLICK_DEPRECATED;
    inline void set_user_anno_u32(int i, uint32_t v) CLICK_DEPRECATED;
    inline int32_t user_anno_s32(int i) const CLICK_DEPRECATED;
    inline void set_user_anno_s32(int i, int32_t v) CLICK_DEPRECATED;
#if HAVE_INT64_TYPES
    inline uint64_t user_anno_u64(int i) const CLICK_DEPRECATED;
    inline void set_user_anno_u64(int i, uint64_t v) CLICK_DEPRECATED;
#endif
    inline const uint8_t *all_user_anno() const CLICK_DEPRECATED;
    inline uint8_t *all_user_anno() CLICK_DEPRECATED;
    inline const uint32_t *all_user_anno_u() const CLICK_DEPRECATED;
    inline uint32_t *all_user_anno_u() CLICK_DEPRECATED;
    inline uint8_t user_anno_c(int) const CLICK_DEPRECATED;
    inline void set_user_anno_c(int, uint8_t) CLICK_DEPRECATED;
    inline int16_t user_anno_s(int) const CLICK_DEPRECATED;
    inline void set_user_anno_s(int, int16_t) CLICK_DEPRECATED;
    inline uint16_t user_anno_us(int) const CLICK_DEPRECATED;
    inline void set_user_anno_us(int, uint16_t) CLICK_DEPRECATED;
    inline int32_t user_anno_i(int) const CLICK_DEPRECATED;
    inline void set_user_anno_i(int, int32_t) CLICK_DEPRECATED;
    inline uint32_t user_anno_u(int) const CLICK_DEPRECATED;
    inline void set_user_anno_u(int, uint32_t) CLICK_DEPRECATED;
    /** @endcond never */
    private:

    // Anno must fit in sk_buff's char cb[48].
    /** @cond never */
    union Anno {
    char c[anno_size];
    uint8_t u8[anno_size];
    uint16_t u16[anno_size / 2];
    uint32_t u32[anno_size / 4];
#if HAVE_INT64_TYPES
    uint64_t u64[anno_size / 8];
#endif
    // allocations: see packet_anno.hh
    };

    struct AllAnno {
    Anno cb;
    unsigned char *mac;
    unsigned char *nh;
    unsigned char *h;
    PacketType pkt_type;
    Timestamp timestamp;
    Packet *next;
    Packet *prev;
//    AllAnno();
//        : timestamp(Timestamp::uninitialized_t()) {
//    }
    AllAnno(){
    }
    };


     // User-space and BSD kernel module implementations.
    atomic_uint32_t _use_count;
    Packet *_data_packet;
    /* mimic Linux sk_buff */
    unsigned char *_head; /* start of allocated buffer */
    unsigned char *_data; /* where the packet starts */
    unsigned char *_tail; /* one beyond end of packet */
    unsigned char *_end;  /* one beyond end of allocated buffer */

    AllAnno _aa;
    buffer_destructor_type _destructor;
    void* _destructor_argument;


    inline Packet() {}
    Packet(const Packet &x);
    ~Packet();
    Packet &operator=(const Packet &x);
    bool alloc_data(uint32_t headroom, uint32_t length, uint32_t tailroom);
    inline void shift_header_annotations(const unsigned char *old_head, int32_t extra_headroom);
    WritablePacket *expensive_uniqueify(int32_t extra_headroom, int32_t extra_tailroom, bool free_on_failure);
    WritablePacket *expensive_push(uint32_t nbytes);
    WritablePacket *expensive_put(uint32_t nbytes);

    friend class WritablePacket;
    friend class PacketBatch;

};

#if HAVE_CLICK_PACKET_POOL
    struct PacketPool {
        WritablePacket* p;          // free packets, linked by p->next()
        unsigned pcount;            // # packets in `p` list
        WritablePacket* pd;             // free data buffers, linked by pd->next
        unsigned pdcount;           // # buffers in `pd` list
    #  if HAVE_MULTITHREAD
        PacketPool* thread_pool_next; // link to next per-thread pool
    #  endif
    };
#endif

class WritablePacket : public Packet { public:
    inline unsigned char *data() const;
    inline unsigned char *end_data() const;
    inline unsigned char *buffer() const;
    inline unsigned char *end_buffer() const;
    inline unsigned char *mac_header() const;
    inline click_ether *ether_header() const;
    inline unsigned char *network_header() const;
    inline click_ip *ip_header() const;
    inline click_ip6 *ip6_header() const;
    inline unsigned char *transport_header() const;
    inline click_icmp *icmp_header() const;
    inline click_tcp *tcp_header() const;
    inline click_udp *udp_header() const;

    /** @cond never */
    inline unsigned char *buffer_data() const CLICK_DEPRECATED;
    /** @endcond never */

 private:

    inline WritablePacket() { }


    inline void initialize();

    WritablePacket(const Packet &x);
    ~WritablePacket() { }

#if HAVE_CLICK_PACKET_POOL
    static WritablePacket *pool_allocate();
    static WritablePacket *pool_data_allocate();
    static WritablePacket *pool_allocate(uint32_t headroom, uint32_t length,
                     uint32_t tailroom);

    static void check_data_pool_size(PacketPool &packet_pool);
    static void check_packet_pool_size(PacketPool &packet_pool);
    static bool is_from_data_pool(WritablePacket *p);
    static void recycle(WritablePacket *p);
    static WritablePacket *pool_batch_allocate(uint16_t count);
    static void recycle_packet_batch(WritablePacket *head, Packet* tail, unsigned count);
    static void recycle_data_batch(WritablePacket *head, Packet* tail, unsigned count);
#endif

    friend class Packet;
    friend class PacketBatch;

};

/** @brief Clear all packet annotations.
 * @param  all  If true, clear all annotations.  If false, clear only Click's
 *   internal annotations.
 *
 * All user annotations and the address annotation are set to zero, the packet
 * type annotation is set to HOST, the device annotation and all header
 * pointers are set to null, the timestamp annotation is cleared, and the
 * next/prev-packet annotations are set to null.
 *
 * If @a all is false, then the packet type, device, timestamp, header, and
 * next/prev-packet annotations are left alone.
 */
inline void
Packet::clear_annotations(bool all)
{
    memset(&_aa, 0, all ? sizeof(AllAnno) : sizeof(Anno));
}

/** @brief Copy most packet annotations from @a p.
 * @param p source of annotations
 *
 * This packet's user annotations, address annotation, packet type annotation,
 * device annotation, and timestamp annotation are set to the corresponding
 * annotations from @a p.
 *
 * @note The next/prev-packet and header annotations are not copied. */
inline void
Packet::copy_annotations(const Packet *p)
{
    *xanno() = *p->xanno();
    set_packet_type_anno(p->packet_type_anno());
    //set_device_anno(p->device_anno());//no element's function call this@zyh
    set_timestamp_anno(p->timestamp_anno());
}

inline void
WritablePacket::initialize()
{
//    _use_count = 1;
    _data_packet = 0;
    _destructor = 0;
    clear_annotations();
}

/** @brief Return the packet's data pointer.
 *
 * This is the pointer to the first byte of packet data. */
inline const unsigned char *
Packet::data() const
{
    return _data;
}


/** @brief Return the packet's end data pointer.
 *
 * The result points at the byte following the packet data.
 * @invariant end_data() == data() + length() */
inline const unsigned char *
Packet::end_data() const
{
    return _tail;
}

/** @brief Return a pointer to the packet's data buffer.
 *
 * The result points at the packet's headroom, not its data.
 * @invariant buffer() == data() - headroom() */
inline const unsigned char *
Packet::buffer() const
{
    return _head;
}


/** @brief Return the packet's end data buffer pointer.
 *
 * The result points past the packet's tailroom.
 * @invariant end_buffer() == end_data() + tailroom() */
inline const unsigned char *
Packet::end_buffer() const
{
    return _end;
}

/** @brief Return the packet's length. */
inline uint32_t
Packet::length() const
{
    return _tail - _data;
}

/** @brief Return the packet's headroom.
 *
 * The headroom is the amount of space available in the current packet buffer
 * before data().  A push() operation is cheap if the packet's unshared and
 * the length pushed is less than headroom(). */
inline uint32_t
Packet::headroom() const
{
    return data() - buffer();
}

/** @brief Return the packet's tailroom.
 *
 * The tailroom is the amount of space available in the current packet buffer
 * following end_data().  A put() operation is cheap if the packet's unshared
 * and the length put is less than tailroom(). */
inline uint32_t
Packet::tailroom() const
{
    return end_buffer() - end_data();
}

/** @brief Return the packet's buffer length.
 * @invariant buffer_length() == headroom() + length() + tailroom()
 * @invariant buffer() + buffer_length() == end_buffer() */
inline uint32_t
Packet::buffer_length() const
{
    return end_buffer() - buffer();
}

inline Packet *
Packet::next() const
{
    return _aa.next;
}

inline Packet *&
Packet::next()
{
    return _aa.next;
}

inline void
Packet::set_next(Packet *p)
{
    _aa.next = p;
}



inline Packet *
Packet::prev() const
{
    return _aa.prev;
}

inline Packet *&
Packet::prev()
{
    return _aa.prev;
}

inline void
Packet::set_prev(Packet *p)
{
    _aa.prev = p;
}

/** @brief Return true iff the packet's MAC header pointer is set.
 * @sa set_mac_header, clear_mac_header */
inline bool
Packet::has_mac_header() const
{
    return _aa.mac != 0;
}



/** @brief Return the packet's MAC header pointer.
 * @warning Not useful if !has_mac_header().
 * @sa ether_header, set_mac_header, clear_mac_header, mac_header_length,
 * mac_length */
inline const unsigned char *
Packet::mac_header() const
{
    return _aa.mac;
}

/** @brief Return true iff the packet's network header pointer is set.
 * @sa set_network_header, clear_network_header */
inline bool
Packet::has_network_header() const
{
    return _aa.nh != 0;
}

/** @brief Return the packet's network header pointer.
 * @warning Not useful if !has_network_header().
 * @sa ip_header, ip6_header, set_network_header, clear_network_header,
 * network_header_length, network_length */
inline const unsigned char *
Packet::network_header() const
{
    return _aa.nh;
}

/** @brief Return true iff the packet's network header pointer is set.
 * @sa set_network_header, clear_transport_header */
inline bool
Packet::has_transport_header() const
{
    return _aa.h != 0;
}

/** @brief Return the packet's transport header pointer.
 * @warning Not useful if !has_transport_header().
 * @sa tcp_header, udp_header, icmp_header, set_transport_header,
 * clear_transport_header, transport_length */
inline const unsigned char *
Packet::transport_header() const
{
    return _aa.h;
}

/** @brief Return the packet's MAC header pointer as Ethernet.
 * @invariant (void *) ether_header() == (void *) mac_header()
 * @warning Not useful if !has_mac_header().
 * @sa mac_header */
inline const click_ether *
Packet::ether_header() const
{
    return reinterpret_cast<const click_ether *>(mac_header());
}

/** @brief Return the packet's network header pointer as IPv4.
 * @invariant (void *) ip_header() == (void *) network_header()
 * @warning Not useful if !has_network_header().
 * @sa network_header */
inline const click_ip *
Packet::ip_header() const
{
    return reinterpret_cast<const click_ip *>(network_header());
}

/** @brief Return the packet's network header pointer as IPv6.
 * @invariant (void *) ip6_header() == (void *) network_header()
 * @warning Not useful if !has_network_header().
 * @sa network_header */
inline const click_ip6 *
Packet::ip6_header() const
{
    return reinterpret_cast<const click_ip6 *>(network_header());
}

/** @brief Return the packet's transport header pointer as ICMP.
 * @invariant (void *) icmp_header() == (void *) transport_header()
 * @warning Not useful if !has_transport_header().
 * @sa transport_header */
inline const click_icmp *
Packet::icmp_header() const
{
    return reinterpret_cast<const click_icmp *>(transport_header());
}

/** @brief Return the packet's transport header pointer as TCP.
 * @invariant (void *) tcp_header() == (void *) transport_header()
 * @warning Not useful if !has_transport_header().
 * @sa transport_header */
inline const click_tcp *
Packet::tcp_header() const
{
    return reinterpret_cast<const click_tcp *>(transport_header());
}

/** @brief Return the packet's transport header pointer as UDP.
 * @invariant (void *) udp_header() == (void *) transport_header()
 * @warning Not useful if !has_transport_header().
 * @sa transport_header */
inline const click_udp *
Packet::udp_header() const
{
    return reinterpret_cast<const click_udp *>(transport_header());
}

/** @brief Return the packet's length starting from its MAC header pointer.
 * @invariant mac_length() == end_data() - mac_header()
 * @warning Not useful if !has_mac_header(). */
inline int
Packet::mac_length() const
{
    return end_data() - mac_header();
}

/** @brief Return the packet's length starting from its network header pointer.
 * @invariant network_length() == end_data() - network_header()
 * @warning Not useful if !has_network_header(). */
inline int
Packet::network_length() const
{
    return end_data() - network_header();
}

/** @brief Return the packet's length starting from its transport header pointer.
 * @invariant transport_length() == end_data() - transport_header()
 * @warning Not useful if !has_transport_header(). */
inline int
Packet::transport_length() const
{
    return end_data() - transport_header();
}

inline const Timestamp &
Packet::timestamp_anno() const
{
    return _aa.timestamp;
}

inline Timestamp &
Packet::timestamp_anno()
{
    return _aa.timestamp;
}

inline void
Packet::set_timestamp_anno(const Timestamp &timestamp)
{
    timestamp_anno() = timestamp;
}



inline Packet::PacketType
Packet::packet_type_anno() const
{
    return _aa.pkt_type;
}

inline void
Packet::set_packet_type_anno(PacketType p)
{
    _aa.pkt_type = p;
}

/** @brief Create and return a new packet.
 * @param data data to be copied into the new packet
 * @param length length of packet
 * @return new packet, or null if no packet could be created
 *
 * The @a data is copied into the new packet.  If @a data is null, the
 * packet's data is left uninitialized.  The new packet's headroom equals
 * @link Packet::default_headroom default_headroom @endlink, its tailroom is 0.
 *
 * The returned packet's annotations are cleared and its header pointers are
 * null. */
inline WritablePacket *
Packet::make(const void *data, uint32_t length)
{
    return make(default_headroom, data, length, 0);
}

/** @brief Create and return a new packet.
 * @param length length of packet
 * @return new packet, or null if no packet could be created
 *
 * The packet's data is left uninitialized.  The new packet's headroom equals
 * @link Packet::default_headroom default_headroom @endlink, its tailroom is 0.
 *
 * The returned packet's annotations are cleared and its header pointers are
 * null. */
inline WritablePacket *
Packet::make(uint32_t length)
{
    return make(default_headroom, (const unsigned char *) 0, length, 0);
}


/** @brief Delete this packet.
 *
 * The packet header (including annotations) is destroyed and its memory
 * returned to the system.  The packet's data is also freed if this is the
 * last clone. */

inline void
Packet::kill()
{
#if HAVE_CLICK_PACKET_POOL
    if (_use_count.dec_and_test())
    WritablePacket::recycle(static_cast<WritablePacket *>(this));
#else
    if (_use_count.dec_and_test())
    delete this;
#endif
}

/** @brief Delete this packet in a thread-safe context
 *
 * The packet header (including annotations) is destroyed and its memory
 * returned to the system.  The packet's data is also freed if this is the
 * last clone.
 *
 * @precond Packet are only handled by this thread */
inline void
Packet::kill_nonatomic()
{
#if CLICK_LINUXMODULE
        struct sk_buff *b = skb();
        b->next = b->prev = 0;
    # if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 15)
        b->list = 0;
    # endif
        skbmgr_recycle_skbs(b);
#elif CLICK_PACKET_USE_DPDK
        rte_pktmbuf_free(mb());
#elif HAVE_CLICK_PACKET_POOL
        //if (_use_count.nonatomic_dec_and_test()) {
            WritablePacket::recycle(static_cast<WritablePacket *>(this));

    //}
//#else
       // if (_use_count.nonatomic_dec_and_test()) {
         //   delete this;
       // }
#endif
}

/** @brief Test whether this packet's data is shared.
 *
 * Returns true iff the packet's data is shared.  If shared() is false, then
 * the result of uniqueify() will equal @c this. */

inline bool
Packet::shared() const
{
    return (_data_packet || _use_count > 1);
}

/** @brief Return an unshared packet containing this packet's data.
 * @return the unshared packet, which is writable
 *
 * The returned packet's data is unshared with any other packet, so it's safe
 * to write the data.  If shared() is false, this operation simply returns the
 * input packet.  If shared() is true, uniqueify() makes a copy of the data.
 * The input packet is freed if the copy fails.
 *
 * The returned WritablePacket pointer may not equal the input Packet pointer,
 * so do not use the input pointer after the uniqueify() call.
 *
 * The input packet's headroom and tailroom areas are copied in addition to
 * its true contents.  The header annotations are shifted to point into the
 * new packet data if necessary.
 *
 * uniqueify() is usually used like this:
 * @code
 * WritablePacket *q = p->uniqueify();
 * if (!q)
 *     return 0;
 * // p must not be used here.
 * @endcode
 */
inline WritablePacket *
Packet::uniqueify()
{
    if (!shared())
    return static_cast<WritablePacket *>(this);
    else
    return expensive_uniqueify(0, 0, true);
}

inline WritablePacket *
Packet::push(uint32_t len)
{
    if (headroom() >= len && !shared()) {
    WritablePacket *q = (WritablePacket *)this;
    /* User-space and BSD kernel module */
    q->_data -= len;
    return q;
    } else
    return expensive_push(len);
}

inline Packet *
Packet::nonunique_push(uint32_t len)
{
    if (headroom() >= len) {
    /* User-space and BSD kernel module */
    _data -= len;
    return this;
    } else
    return expensive_push(len);
}

inline void
Packet::pull(uint32_t len)
{
    if (len > length()) {
    //click_chatter("Packet::pull %d > length %d\n", len, length());//report error@zyh
    len = length();
    }
    /* User-space and BSD kernel module */
    _data += len;
}

inline WritablePacket *
Packet::put(uint32_t len)
{
    if (tailroom() >= len && !shared()) {
    WritablePacket *q = (WritablePacket *)this;
    /* User-space and BSD kernel module */
    q->_tail += len;

    return q;
    } else
    return expensive_put(len);
}

inline Packet *
Packet::nonunique_put(uint32_t len)
{
    if (tailroom() >= len) {
   /* User-space and BSD kernel module */
    _tail += len;

    return this;
    } else
    return expensive_put(len);
}

inline void
Packet::take(uint32_t len)
{
    if (len > length()) {
    //click_chatter("Packet::take %d > length %d\n", len, length());//report error@zyh
    len = length();
    }
    /* User-space and BSD kernel module */
    _tail -= len;
}


/** @brief Shrink the packet's data.
 * @param data new data pointer
 * @param length new length
 *
 * @warning This function is useful only in special contexts.
 * @note Only available at user level
 *
 * User-level programs that read packet logs commonly read a large chunk of
 * data (32 kB or more) into a base Packet object.  The log reader then works
 * over the data buffer and, for each packet contained therein, outputs a
 * clone that shares memory with the base packet.  This is space- and
 * time-efficient, but the generated packets have gigantic headroom and
 * tailroom.  Uniqueifying a generated packet will wastefully copy this
 * headroom and tailroom as well.  The shrink_data function addresses this
 * problem.
 *
 * shrink_data() removes all of a packet's headroom and tailroom.  The
 * resulting packet has data() equal to @a data, length() equal to @a length,
 * and headroom() and tailroom() equal to zero.
 *
 * @pre The packet @em must be a clone() of another existing packet.
 * @pre @a data >= data(), @a data <= end_data(), @a data + @a length >=
 * data(), and @a data + @a length <= end_data()
 *
 * @sa change_headroom_and_length */
inline void
Packet::shrink_data(const unsigned char *data, uint32_t length)
{
    assert(_data_packet);
    if (data >= _head && data + length >= data && data + length <= _end) {
    _head = _data = const_cast<unsigned char *>(data);
    _tail = _end = const_cast<unsigned char *>(data + length);
    }
}

/** @brief Shift the packet's data view to a different part of its buffer.
 * @param headroom new headroom
 * @param length new length
 *
 * @warning This function is useful only in special contexts.
 * @note Only available at user level
 *
 * Shifts the packet's data() pointer to a different part of the packet's data
 * buffer.  The buffer pointer itself is not changed, and the packet's
 * contents are not affected (except by the new view).
 *
 * @pre @a headroom + @a length <= buffer_length()
 * @post new buffer() == old buffer()
 * @post new end_buffer() == old end_buffer()
 * @post new headroom() == @a headroom
 * @post new length() == @a length
 *
 * @sa shrink_data */
inline void
Packet::change_headroom_and_length(uint32_t headroom, uint32_t length)
{
    if (headroom + length <= buffer_length()) {
    _data = _head + headroom;
    _tail = _data + length;
    }
}

inline IPAddress
Packet::dst_ip_anno() const
{
    return IPAddress(xanno()->u32[dst_ip_anno_offset / 4]);
}

inline void
Packet::set_dst_ip_anno(IPAddress a)
{
    xanno()->u32[dst_ip_anno_offset / 4] = a.addr();
}

/** @brief Set the MAC header pointer.
 * @param p new header pointer */
inline void
Packet::set_mac_header(const unsigned char *p)
{
    assert(p >= buffer() && p <= end_buffer());

  /* User-space and BSD kernel module */
    _aa.mac = const_cast<unsigned char *>(p);

}

/** @brief Set the MAC and network header pointers.
 * @param p new MAC header pointer
 * @param len new MAC header length
 * @post mac_header() == @a p and network_header() == @a p + @a len */
inline void
Packet::set_mac_header(const unsigned char *p, uint32_t len)
{
    assert(p >= buffer() && p + len <= end_buffer());
      /* User-space and BSD kernel module */
    _aa.mac = const_cast<unsigned char *>(p);
    _aa.nh = const_cast<unsigned char *>(p) + len;

}

/** @brief Set the MAC header pointer to an Ethernet header.
 * @param ethh new Ethernet header pointer
 * @post (void *) mac_header() == (void *) @a ethh
 * @post mac_header_length() == 14
 * @post (void *) network_header() == (void *) (@a ethh + 1) */
inline void
Packet::set_ether_header(const click_ether *ethh)
{
    set_mac_header(reinterpret_cast<const unsigned char *>(ethh), 14);
}

/** @brief Unset the MAC header pointer.
 * @post has_mac_header() == false
 * Does not affect the network or transport header pointers. */
inline void
Packet::clear_mac_header()
{
      /* User-space and BSD kernel module */
    _aa.mac = 0;

}

inline WritablePacket *
Packet::push_mac_header(uint32_t len)
{
    WritablePacket *q;
    if (headroom() >= len && !shared()) {
    q = (WritablePacket *)this;
    /* User-space and BSD kernel module */
    q->_data -= len;

    } else if ((q = expensive_push(len)))
    /* nada */;
    else
    return 0;
    q->set_mac_header(q->data(), len);
    return q;
}


inline void
Packet::set_network_header(const unsigned char *p, uint32_t len)
{
    assert(p >= buffer() && p + len <= end_buffer());       
       /* User-space and BSD kernel module */
    _aa.nh = const_cast<unsigned char *>(p);
    _aa.h = const_cast<unsigned char *>(p) + len;

}

/** @brief Set the network header length.
 * @param len new network header length
 *
 * Setting the network header length really just sets the transport header
 * pointer.
 * @post transport_header() == network_header() + @a len */
inline void
Packet::set_network_header_length(uint32_t len)
{
    assert(network_header() + len <= end_buffer());
    /* User-space and BSD kernel module */
    _aa.h = _aa.nh + len;

}

/** @brief Set the network header pointer to an IPv4 header.
 * @param iph new IP header pointer
 * @param len new IP header length in bytes
 * @post (char *) network_header() == (char *) @a iph
 * @post network_header_length() == @a len
 * @post (char *) transport_header() == (char *) @a iph + @a len */
inline void
Packet::set_ip_header(const click_ip *iph, uint32_t len)
{
    set_network_header(reinterpret_cast<const unsigned char *>(iph), len);
}

/** @brief Set the network header pointer to an IPv6 header.
 * @param ip6h new IP header pointer
 * @param len new IP header length in bytes
 * @post (char *) network_header() == (char *) @a ip6h
 * @post network_header_length() == @a len
 * @post (char *) transport_header() == (char *) @a ip6h + @a len */
inline void
Packet::set_ip6_header(const click_ip6 *ip6h, uint32_t len)
{
    set_network_header(reinterpret_cast<const unsigned char *>(ip6h), len);
}

/** @brief Set the network header pointer to an IPv6 header.
 * @param ip6h new IP header pointer
 * @post (char *) network_header() == (char *) @a ip6h
 * @post network_header_length() == 40
 * @post (char *) transport_header() == (char *) (@a ip6h + 1) */
inline void
Packet::set_ip6_header(const click_ip6 *ip6h)
{
    set_ip6_header(ip6h, 40);
}

/** @brief Unset the network header pointer.
 * @post has_network_header() == false
 * Does not affect the MAC or transport header pointers. */
inline void
Packet::clear_network_header()
{
    /* User-space and BSD kernel module */
    _aa.nh = 0;
}

/** @brief Return the offset from the packet data to the MAC header.
 * @return mac_header() - data()
 * @warning Not useful if !has_mac_header(). */
inline int
Packet::mac_header_offset() const
{
    return mac_header() - data();
}

/** @brief Return the MAC header length.
 * @return network_header() - mac_header()
 *
 * This equals the offset from the MAC header pointer to the network header
 * pointer.
 * @warning Not useful if !has_mac_header() or !has_network_header(). */
inline uint32_t
Packet::mac_header_length() const
{
    return network_header() - mac_header();
}

/** @brief Return the offset from the packet data to the network header.
 * @return network_header() - data()
 * @warning Not useful if !has_network_header(). */
inline int
Packet::network_header_offset() const
{
    return network_header() - data();
}

/** @brief Return the network header length.
 * @return transport_header() - network_header()
 *
 * This equals the offset from the network header pointer to the transport
 * header pointer.
 * @warning Not useful if !has_network_header() or !has_transport_header(). */
inline uint32_t
Packet::network_header_length() const
{
    return transport_header() - network_header();
}

/** @brief Return the offset from the packet data to the IP header.
 * @return network_header() - mac_header()
 * @warning Not useful if !has_network_header().
 * @sa network_header_offset */
inline int
Packet::ip_header_offset() const
{
    return network_header_offset();
}

/** @brief Return the IP header length.
 * @return transport_header() - network_header()
 *
 * This equals the offset from the network header pointer to the transport
 * header pointer.
 * @warning Not useful if !has_network_header() or !has_transport_header().
 * @sa network_header_length */
inline uint32_t
Packet::ip_header_length() const
{
    return network_header_length();
}

/** @brief Return the offset from the packet data to the IPv6 header.
 * @return network_header() - data()
 * @warning Not useful if !has_network_header().
 * @sa network_header_offset */
inline int
Packet::ip6_header_offset() const
{
    return network_header_offset();
}

/** @brief Return the IPv6 header length.
 * @return transport_header() - network_header()
 *
 * This equals the offset from the network header pointer to the transport
 * header pointer.
 * @warning Not useful if !has_network_header() or !has_transport_header().
 * @sa network_header_length */
inline uint32_t
Packet::ip6_header_length() const
{
    return network_header_length();
}

/** @brief Return the offset from the packet data to the transport header.
 * @return transport_header() - data()
 * @warning Not useful if !has_transport_header(). */
inline int
Packet::transport_header_offset() const
{
    return transport_header() - data();
}

/** @brief Unset the transport header pointer.
 * @post has_transport_header() == false
 * Does not affect the MAC or network header pointers. */
inline void
Packet::clear_transport_header()
{
    /* User-space and BSD kernel module */
    _aa.h = 0;
}

inline void
Packet::shift_header_annotations(const unsigned char *old_head,
                 int32_t extra_headroom)
{
    ptrdiff_t shift = (_head - old_head) + extra_headroom;
    _aa.mac += (_aa.mac ? shift : 0);
    _aa.nh += (_aa.nh ? shift : 0);
    _aa.h += (_aa.h ? shift : 0);

}

/** @cond never */
/** @brief Return a pointer to the packet's data buffer.
 * @deprecated Use buffer() instead. */
inline const unsigned char *
Packet::buffer_data() const
{
    return _head;

}

/** @brief Return a pointer to the address annotation area.
 * @deprecated Use anno() instead.
 *
 * The area is ADDR_ANNO_SIZE bytes long. */
inline void *Packet::addr_anno() {
    return anno_u8() + addr_anno_offset;
}

/** @overload */
inline const void *Packet::addr_anno() const {
    return anno_u8() + addr_anno_offset;
}

/** @brief Return a pointer to the user annotation area.
 * @deprecated Use Packet::anno() instead.
 *
 * The area is USER_ANNO_SIZE bytes long. */
inline void *Packet::user_anno() {
    return anno_u8() + user_anno_offset;
}

/** @overload */
inline const void *Packet::user_anno() const {
    return anno_u8() + user_anno_offset;
}

/** @brief Return a pointer to the user annotation area as uint8_ts.
 * @deprecated Use Packet::anno_u8() instead. */
inline uint8_t *Packet::user_anno_u8() {
    return anno_u8() + user_anno_offset;
}

/** @brief overload */
inline const uint8_t *Packet::user_anno_u8() const {
    return anno_u8() + user_anno_offset;
}

/** @brief Return a pointer to the user annotation area as uint32_ts.
 * @deprecated Use Packet::anno_u32() instead. */
inline uint32_t *Packet::user_anno_u32() {
    return anno_u32() + user_anno_offset / 4;
}

/** @brief overload */
inline const uint32_t *Packet::user_anno_u32() const {
    return anno_u32() + user_anno_offset / 4;
}

/** @brief Return user annotation byte @a i.
 * @param i annotation index
 * @pre 0 <= @a i < USER_ANNO_SIZE
 * @deprecated Use Packet::anno_u8(@a i) instead. */
inline uint8_t Packet::user_anno_u8(int i) const {
    return anno_u8(user_anno_offset + i);
}

/** @brief Set user annotation byte @a i.
 * @param i annotation index
 * @param v value
 * @pre 0 <= @a i < USER_ANNO_SIZE
 * @deprecated Use Packet::set_anno_u8(@a i, @a v) instead. */
inline void Packet::set_user_anno_u8(int i, uint8_t v) {
    set_anno_u8(user_anno_offset + i, v);
}

/** @brief Return 16-bit user annotation @a i.
 * @param i annotation index
 * @pre 0 <= @a i < USER_ANNO_U16_SIZE
 * @deprecated Use Packet::anno_u16(@a i * 2) instead.
 *
 * Affects user annotation bytes [2*@a i, 2*@a i+1]. */
inline uint16_t Packet::user_anno_u16(int i) const {
    return anno_u16(user_anno_offset + i * 2);
}

/** @brief Set 16-bit user annotation @a i.
 * @param i annotation index
 * @param v value
 * @pre 0 <= @a i < USER_ANNO_U16_SIZE
 * @deprecated Use Packet::set_anno_u16(@a i * 2, @a v) instead.
 *
 * Affects user annotation bytes [2*@a i, 2*@a i+1]. */
inline void Packet::set_user_anno_u16(int i, uint16_t v) {
    set_anno_u16(user_anno_offset + i * 2, v);
}

/** @brief Return 32-bit user annotation @a i.
 * @param i annotation index
 * @pre 0 <= @a i < USER_ANNO_U32_SIZE
 * @deprecated Use Packet::anno_u32(@a i * 4) instead.
 *
 * Affects user annotation bytes [4*@a i, 4*@a i+3]. */
inline uint32_t Packet::user_anno_u32(int i) const {
    return anno_u32(user_anno_offset + i * 4);
}

/** @brief Set 32-bit user annotation @a i.
 * @param i annotation index
 * @param v value
 * @pre 0 <= @a i < USER_ANNO_U32_SIZE
 * @deprecated Use Packet::set_anno_u32(@a i * 4, @a v) instead.
 *
 * Affects user annotation bytes [4*@a i, 4*@a i+3]. */
inline void Packet::set_user_anno_u32(int i, uint32_t v) {
    set_anno_u32(user_anno_offset + i * 4, v);
}

/** @brief Return 32-bit user annotation @a i.
 * @param i annotation index
 * @pre 0 <= @a i < USER_ANNO_U32_SIZE
 * @deprecated Use Packet::anno_s32(@a i * 4) instead.
 *
 * Affects user annotation bytes [4*@a i, 4*@a i+3]. */
inline int32_t Packet::user_anno_s32(int i) const {
    return anno_s32(user_anno_offset + i * 4);
}

/** @brief Set 32-bit user annotation @a i.
 * @param i annotation index
 * @param v value
 * @pre 0 <= @a i < USER_ANNO_U32_SIZE
 * @deprecated Use Packet::set_anno_s32(@a i * 4, @a v) instead.
 *
 * Affects user annotation bytes [4*@a i, 4*@a i+3]. */
inline void Packet::set_user_anno_s32(int i, int32_t v) {
    set_anno_s32(user_anno_offset + i * 4, v);
}

#if HAVE_INT64_TYPES
/** @brief Return 64-bit user annotation @a i.
 * @param i annotation index
 * @pre 0 <= @a i < USER_ANNO_U64_SIZE
 * @deprecated Use Packet::anno_u64(@a i * 8) instead.
 *
 * Affects user annotation bytes [8*@a i, 8*@a i+7]. */
inline uint64_t Packet::user_anno_u64(int i) const {
    return anno_u64(user_anno_offset + i * 8);
}

/** @brief Set 64-bit user annotation @a i.
 * @param i annotation index
 * @param v value
 * @pre 0 <= @a i < USER_ANNO_U64_SIZE
 * @deprecated Use Packet::set_anno_u64(@a i * 8, @a v) instead.
 *
 * Affects user annotation bytes [8*@a i, 8*@a i+7]. */
inline void Packet::set_user_anno_u64(int i, uint64_t v) {
    set_anno_u64(user_anno_offset + i * 8, v);
}
#endif

/** @brief Return a pointer to the user annotation area.
 * @deprecated Use anno() instead. */
inline const uint8_t *Packet::all_user_anno() const {
    return anno_u8() + user_anno_offset;
}

/** @overload
 * @deprecated Use anno() instead. */
inline uint8_t *Packet::all_user_anno() {
    return anno_u8() + user_anno_offset;
}

/** @brief Return a pointer to the user annotation area as uint32_ts.
 * @deprecated Use anno_u32() instead. */
inline const uint32_t *Packet::all_user_anno_u() const {
    return anno_u32() + user_anno_offset / 4;
}

/** @overload
 * @deprecated Use anno_u32() instead. */
inline uint32_t *Packet::all_user_anno_u() {
    return anno_u32() + user_anno_offset / 4;
}

/** @brief Return user annotation byte @a i.
 * @deprecated Use anno_u8() instead. */
inline uint8_t Packet::user_anno_c(int i) const {
    return anno_u8(user_anno_offset + i);
}

/** @brief Set user annotation byte @a i.
 * @deprecated Use set_anno_u8() instead. */
inline void Packet::set_user_anno_c(int i, uint8_t v) {
    set_anno_u8(user_anno_offset + i, v);
}

/** @brief Return 16-bit user annotation @a i.
 * @deprecated Use anno_u16() instead. */
inline uint16_t Packet::user_anno_us(int i) const {
    return anno_u16(user_anno_offset + i * 2);
}

/** @brief Set 16-bit user annotation @a i.
 * @deprecated Use set_anno_u16() instead. */
inline void Packet::set_user_anno_us(int i, uint16_t v) {
    set_anno_u16(user_anno_offset + i * 2, v);
}

/** @brief Return 16-bit user annotation @a i.
 * @deprecated Use anno_u16() instead. */
inline int16_t Packet::user_anno_s(int i) const {
    return (int16_t) anno_u16(user_anno_offset + i * 2);
}

/** @brief Set 16-bit user annotation @a i.
 * @deprecated Use set_anno_u16() instead. */
inline void Packet::set_user_anno_s(int i, int16_t v) {
    set_anno_u16(user_anno_offset + i * 2, v);
}

/** @brief Return 32-bit user annotation @a i.
 * @deprecated Use anno_u32() instead. */
inline uint32_t Packet::user_anno_u(int i) const {
    return anno_u32(user_anno_offset + i * 4);
}

/** @brief Set 32-bit user annotation @a i.
 * @deprecated Use set_anno_u32() instead. */
inline void Packet::set_user_anno_u(int i, uint32_t v) {
    set_anno_u32(user_anno_offset + i * 4, v);
}

/** @brief Return 32-bit user annotation @a i.
 * @deprecated Use anno_s32() instead. */
inline int32_t Packet::user_anno_i(int i) const {
    return anno_s32(user_anno_offset + i * 4);
}

/** @brief Set 32-bit user annotation @a i.
 * @deprecated Use set_anno_s32() instead. */
inline void Packet::set_user_anno_i(int i, int32_t v) {
    set_anno_s32(user_anno_offset + i * 4, v);
}
/** @endcond never */

inline unsigned char *
WritablePacket::data() const
{
    return const_cast<unsigned char *>(Packet::data());
}

inline unsigned char *
WritablePacket::end_data() const
{
    return const_cast<unsigned char *>(Packet::end_data());
}

inline unsigned char *
WritablePacket::buffer() const
{
    return const_cast<unsigned char *>(Packet::buffer());
}

inline unsigned char *
WritablePacket::end_buffer() const
{
    return const_cast<unsigned char *>(Packet::end_buffer());
}

inline unsigned char *
WritablePacket::mac_header() const
{
    return const_cast<unsigned char *>(Packet::mac_header());
}

inline unsigned char *
WritablePacket::network_header() const
{
    return const_cast<unsigned char *>(Packet::network_header());
}

inline unsigned char *
WritablePacket::transport_header() const
{
    return const_cast<unsigned char *>(Packet::transport_header());
}

inline click_ether *
WritablePacket::ether_header() const
{
    return const_cast<click_ether *>(Packet::ether_header());
}

inline click_ip *
WritablePacket::ip_header() const
{
    return const_cast<click_ip *>(Packet::ip_header());
}

inline click_ip6 *
WritablePacket::ip6_header() const
{
    return const_cast<click_ip6 *>(Packet::ip6_header());
}

inline click_icmp *
WritablePacket::icmp_header() const
{
    return const_cast<click_icmp *>(Packet::icmp_header());
}

inline click_tcp *
WritablePacket::tcp_header() const
{
    return const_cast<click_tcp *>(Packet::tcp_header());
}

inline click_udp *
WritablePacket::udp_header() const
{
    return const_cast<click_udp *>(Packet::udp_header());
}

/** @cond never */
inline unsigned char *
WritablePacket::buffer_data() const
{
    return const_cast<unsigned char *>(Packet::buffer());
}
/** @endcond never */


#endif
