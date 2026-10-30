#pragma once
#include <Arduino.h>

template <class chipType>
class ExpanderSPI
{
public:
    using ChipType = chipType;
    void beginSPI(int mosi,
                  int miso,
                  int sck,
                  int cs)
    {
        _mosi = mosi;
        _miso = miso;
        _sck = sck;
        _cs = cs;

        thisChip().pinMode(_mosi, OUTPUT);
        thisChip().pinMode(_sck, OUTPUT);

        if (_cs != -1)
        {
            thisChip().pinMode(_cs, OUTPUT);
            thisChip().digitalWrite(_cs, HIGH);
        }

        if (_miso != -1)
        {
            thisChip().pinMode(_miso, INPUT);
        }
        thisChip().digitalWrite(_mosi, HIGH);
        thisChip().digitalWrite(_sck, HIGH);
    }

    void setBitOrder(uint8_t order)
    {
        _order = order & 1;
    }

    uint8_t transfer8(uint8_t val)
    {
        return transferDataBits(val, 8);
    }

    uint16_t transfer9(uint16_t val)
    {
        return transferDataBits(val, 9);
    }

    uint32_t transferDataBits(uint32_t val, uint32_t bits)
    {
        uint32_t mask = _BV(bits - 1);
        uint8_t out = 0;
        if (_cs != -1)
        {
            thisChip().digitalWrite(_cs, LOW);
        }
        for (int i = 0; i < bits; i++)
        {
            thisChip().digitalWrite(_sck, LOW);
            out <<= 1;
            if (_miso != -1)
            {
                if (thisChip().digitalRead(_miso))
                {
                    out |= 0x01;
                }
            }
            thisChip().digitalWrite(_mosi, val & mask ? HIGH : LOW);
            val <<= 1;
            thisChip().digitalWrite(_sck, HIGH);
        }
        if (_cs != -1)
        {
            thisChip().digitalWrite(_cs, HIGH);
        }
        return out;
    }

protected:
    inline const chipType &thisChip() const
    {
        return static_cast<const chipType &>(*this);
    }

    inline chipType &thisChip()
    {
        return static_cast<chipType &>(*this);
    }

private:
    int _mosi;
    int _sck;
    int _miso;
    int _order;
    int _cs;
};