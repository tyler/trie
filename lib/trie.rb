require 'trie/trie'

class AlphaMap
  class <<self
    # Return a map with an effective range of 7bit ASCII
    #
    # @return [AlphaMap]
    def ascii
      new.tap { |map| map.add_range(0, 127) }
    end

    # Returns a map limited by the given ranges
    #
    # The following types are supported:
    # - Range (disregarding the exclusiveness attribute)
    # - Fixnum (single ordinal value)
    # - String (single ordinal value for each character, useful for special characters)
    #
    # @param [Array<Range|Fixnum|String>] ranges
    def from_ranges(*ranges)
      new.tap do |map|
        ranges.each do |range|
          case range
          when Range then map.add_range(range.first.ord, range.last.ord)
          when 0.class then map.add_range(range, range)
          when String then range.each_char { |char| map.add_range(char.ord, char.ord) }
          else raise ArgumentError, "#{range} is not supported as an argument"
          end
        end
      end
    end
  end
end
