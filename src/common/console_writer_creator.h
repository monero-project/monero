#pragma once

#include <memory>
#include <unordered_map>

#include "scoped_message_writer.h"

namespace tools
{
    enum class ConsoleWriterMode
    {
        Standard,
        Colorblind,
    };

    class IMessageWriter
    {
    public:
        virtual scoped_message_writer GetSuccessMessageWriter() const = 0;
        virtual scoped_message_writer GetFailureMessageWriter() const = 0;
        virtual scoped_message_writer GetBrightRedMessageWriter() const = 0;
        virtual scoped_message_writer GetBrightGreenMessageWriter() const = 0;
        scoped_message_writer GetMessageWriter() { return GetCustomMessageWriter(epee::console_color_default); }
        scoped_message_writer GetCustomMessageWriter(epee:console_colors custom_color) const { return msg_writer(custom_color); }
        virtual IMessageWriter() = default;
    };

    class StandardMessageWriter : public IMessageWriter
    {
    public:
        scoped_message_writer GetSuccessMessageWriter() const override { return success_msg_writer(); }
        scoped_message_writer GetFailureMessageWriter() const override { return fail_msg_writer(); }
        scoped_message_writer GetBrightRedMessageWriter() const override { return msg_writer(epee::console_color_red, true); }
        scoped_message_writer GetBrightGreenMessageWriter() const override { return msg_writer(epee:console_color_green, true); }
    };

    class ColorblindMessageWriter : public IMessageWriter
    {
    public:
        scoped_message_writer GetSuccessMessageWriter() const override { return msg_writer(epee::console_colorblind_light_green); }
        scoped_message_writer GetFailureMessageWriter() const override { return msg_writer(epee::console_colorblind_light_red); }
        scoped_message_writer GetBrightRedMessageWriter() const override { return msg_writer(epee::console_colorblind_light_red, true); }
        scoped_message_writer GetBrightGreenMessageWriter() const override { return msg_writer(epee::console_colorblind_light_green, true); }
    };

    class MessageWriterFactory
    {
    public:
        static std::shared_ptr<IMessageWriter> GetMessageWriter(ConsoleWriterMode mode)
        {
            static std::unordered_map<ConsoleWriterMode, std::shared_ptr<IMessageWriter>> writers
            {
                { ConsoleWriterMode::Standard, std::make_shared<StandardMessageWriter>() },
                { ConsoleWriterMode::Colorblind, std::make_shared<ColorblindMessageWriter>() }
            };

            if (auto writer = writers.find(mode); writer != writers.end())
            {
                return writer->second;
            }
            else
            {
                throw std::logic_error("Unsupported ConsoleWriterMode");
            }
        }

    };
}
