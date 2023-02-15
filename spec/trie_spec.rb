require File.dirname(__FILE__) + '/../lib/trie'

describe Trie do
  before :each do
    @trie = Trie.new
    @trie.add('rocket')
    @trie.add('rock')
    @trie.add('frederico')
    @trie.add('français')
  end
  
  describe :has_key? do
    it 'returns true for words in the trie' do
      expect(@trie.has_key?('rocket')).to be true
    end

    it 'returns true for non-ASCII words in the trie' do
      expect(@trie.has_key?('français')).to be true
    end

    it 'returns nil for words that are not in the trie' do
      expect(@trie.has_key?('not_in_the_trie')).to be nil
    end
  end

  describe :get do
    it 'returns -1 for words in the trie without a weight' do
      expect(@trie.get('rocket')).to eq(-1)
    end

    it 'returns -1 for non-ASCII words in the trie without a weight' do
      expect(@trie.get('français')).to eq(-1)
    end

    it 'returns nil if the word is not in the trie' do
      expect(@trie.get('not_in_the_trie')).to be nil
    end
  end

  describe :add do
    it 'adds a word to the trie' do
      expect(@trie.add('forsooth')).to be true
      expect(@trie.get('forsooth')).to eq(-1)
    end

    it 'adds a word with a weight to the trie' do
      expect(@trie.add('chicka',123)).to be true
      expect(@trie.get('chicka')).to eq(123)
    end

    it 'adds values greater than 16-bit allows' do
      expect(@trie.add('chicka', 72_000)).to be true
      expect(@trie.get('chicka')).to eq(72_000)
    end

    it 'adds values that are not plain ASCII' do
      expect(@trie.add('café', 72_000)).to be true
      expect(@trie.get('café')).to eq(72_000)
    end
  end

  describe :add_if_absent do
    it 'adds a word to the trie' do
      expect(@trie.add_if_absent('forsooth')).to be true
      expect(@trie.get('forsooth')).to eq(-1)
    end

    it 'not add a word that is already present' do
      expect(@trie.add_if_absent('forsooth')).to be true
      expect(@trie.add_if_absent('forsooth')).to be nil
    end
  end

  describe :add_text do
    it 'adds multiple words to the trie delimited by spaces' do
      @trie.add_text('forsooth chicka boom')
      expect(@trie.has_key?('forsooth')).to be true
      expect(@trie.has_key?('chicka')).to be true
      expect(@trie.has_key?('boom')).to be true
      expect(@trie.has_key?('not_there')).to be nil
    end

    it 'adds multiple words to the trie delimited by commas' do
      @trie.add_text('forsooth,chicka,boom')
      expect(@trie.has_key?('forsooth')).to be true
      expect(@trie.has_key?('chicka')).to be true
      expect(@trie.has_key?('boom')).to be true
      expect(@trie.has_key?('not_there')).to be nil
    end
  end

  describe :add_tags do
    it 'adds multiple words to the trie delimited by spaces' do
      @trie.add_tags('forsooth chicka boom')
      expect(@trie.has_key?('forsooth')).to be true
      expect(@trie.has_key?('chicka')).to be true
      expect(@trie.has_key?('boom')).to be true
      expect(@trie.has_key?('not_there')).to be nil
    end
  end

  describe :concat do
    it 'adds multiple words to the trie' do
      @trie.concat %w{forsooth chicka boom}
      expect(@trie.has_key?('forsooth')).to be true
      expect(@trie.has_key?('chicka')).to be true
      expect(@trie.has_key?('boom')).to be true
      expect(@trie.has_key?('not_there')).to be nil
    end

    it 'adds multiple words to the trie with weights' do
      @trie.concat [['forsooth', 1], ['chicka', 2], ['boom', 3]]
      expect(@trie.get('forsooth')).to eq(1)
      expect(@trie.get('chicka')).to eq(2)
      expect(@trie.get('boom')).to eq(3)
      expect(@trie.has_key?('not_there')).to be nil
    end
  end

  describe :text_has_keys? do
    it 'scans words in text for keys' do
      @trie.add_text('forsooth rock rocket')
      expect(@trie.text_has_keys?('the word "forsooth" is in this text')).to be true
      expect(@trie.text_has_keys?('yes, rock!')).to be true
      expect(@trie.text_has_keys?('3.rocket.4')).to be true
      expect(@trie.text_has_keys?('not_there not_there_either')).to be nil
    end
  end

  describe :tags_has_keys? do
    it 'scans words in space-delimited tag list for keys' do
      @trie.add_tags('forsooth rock rocket')
      expect(@trie.tags_has_keys?('banana frobozz rock')).to be true
      expect(@trie.tags_has_keys?('rocket abc def')).to be true
      expect(@trie.tags_has_keys?('yes forsooth group')).to be true
      expect(@trie.tags_has_keys?('not_there not_there_either')).to be nil
    end
  end

  describe :delete do
    it 'deletes a word from the trie' do
      expect(@trie.delete('rocket')).to be true
      expect(@trie.has_key?('rocket')).to be nil
    end

    it 'deletes a non-ASCII word from the trie' do
      expect(@trie.delete('français')).to be true
      expect(@trie.has_key?('français')).to be nil
    end
  end

  describe :children do
    it 'returns all words beginning with a given prefix' do
      children = @trie.children('roc')
      expect(children.size).to eq(2)
      expect(children).to include('rock')
      expect(children).to include('rocket')
    end

    it 'accepts non-ASCII prefixes and returns non-ASCII words' do
      @trie.add('café')
      @trie.add('cafétière')
      children = @trie.children('café')
      expect(children.size).to eq(2)
      expect(children).to include('café')
      expect(children).to include('cafétière')
    end

    it 'returns blank array if prefix does not exist' do
      expect(@trie.children('ajsodij')).to eq([])
    end

    it 'includes the prefix if the prefix is a word' do
      children = @trie.children('rock')
      expect(children.size).to eq(2)
      expect(children).to include('rock')
      expect(children).to include('rocket')
    end

    it 'returns blank array if prefix is nil' do
      expect(@trie.children(nil)).to eq([])
    end
  end

  describe :children_with_values do
    before :each do
      @trie.add('abc',2)
      @trie.add('abcd',4)
    end

    it 'returns all words with values beginning with a given prefix' do
      children = @trie.children_with_values('ab')
      expect(children.size).to eq(2)
      expect(children).to include(['abc',2])
      expect(children).to include(['abcd',4])
    end

    it 'returns all words with values beginning with a given non-ASCII prefix' do
      @trie.add('café', 2)
      @trie.add('cafétière', 4)
      children = @trie.children_with_values('café')
      expect(children.size).to eq(2)
      expect(children).to include(['café',2])
      expect(children).to include(['cafétière',4])
    end

    it 'returns nil if prefix does not exist' do
      expect(@trie.children_with_values('ajsodij')).to eq([])
    end

    it 'includes the prefix if the prefix is a word' do
      children = @trie.children_with_values('abc')
      expect(children.size).to eq(2)
      expect(children).to include(['abc',2])
      expect(children).to include(['abcd',4])
    end

    it 'returns blank array if prefix is nil' do
      expect(@trie.children_with_values(nil)).to eq([])
    end
  end

  #describe :walk_to_terminal do
  #  it 'returns the first word found along a path' do
  #    @trie.add 'anderson'
  #    @trie.add 'andreas'
  #    @trie.add 'and'

  #    expect(@trie.walk_to_terminal('anderson')).to eq('and')
  #  end

  #  it 'returns the first word and value along a path' do
  #    @trie.add 'anderson'
  #    @trie.add 'andreas'
  #    @trie.add 'and', 15

  #    expect(@trie.walk_to_terminal('anderson',true)).to eq(['and', 15])
  #  end
  #end

  describe :root do
    it 'returns a TrieNode' do
      expect(@trie.root).to be_a TrieNode
    end

    it 'returns a different TrieNode each time' do
      expect(@trie.root).not_to eq(@trie.root)
    end
  end

  describe 'save/read' do
    let(:filename_base) do
      dir = File.expand_path(File.join(File.dirname(__FILE__), '..', 'tmp'))
      FileUtils.mkdir_p(dir)
      File.join(dir, 'trie')
    end

    context 'when I save the populated trie to disk' do
      before(:each) do
        @trie.add('omgwtflolbbq', 123)
        @trie.save(filename_base)
      end

      it 'contains the same data when reading from disk' do
        trie2 = Trie.read(filename_base)
        expect(trie2.get('omgwtflolbbq')).to eq(123)
      end
    end
  end

  describe :read do
    context 'when the files to read from do not exist' do
      let(:filename_base) do
        "phantasy/file/path/that/does/not/exist"
      end

      it 'raises an error when attempting a read' do
        expect { Trie.read(filename_base) }.to raise_error(IOError)
      end
    end
  end

  describe :has_children? do
    it 'returns true when there are children matching prefix' do
      expect(@trie.has_children?('r')).to be true
      expect(@trie.has_children?('rock')).to be true
      expect(@trie.has_children?('rocket')).to be true
    end

    it 'returns true when there are children matching non-ASCII prefix' do
      expect(@trie.has_children?('franç')).to be true
    end

    it 'returns false when there are no children matching prefix' do
      expect(@trie.has_children?('no')).to be false
      expect(@trie.has_children?('rome')).to be false
      expect(@trie.has_children?('roc_')).to be false
    end
  end
end

describe TrieNode do
  before :each do
    @trie = Trie.new
    @trie.add('rocket',1)
    @trie.add('rock',2)
    @trie.add('frederico',3)
    @trie.add('café',4)
    @node = @trie.root
  end
  
  describe :state do
    it 'returns the most recent state character' do
      @node.walk!('r')
      expect(@node.state).to eq('r')
      @node.walk!('o')
      expect(@node.state).to eq('o')
    end

    it 'is nil when no walk has occurred' do
      expect(@node.state).to be nil
    end
  end

  describe :full_state do
    it 'returns the current string' do
      @node.walk!('r').walk!('o').walk!('c')
      expect(@node.full_state).to eq('roc')
    end

    it 'is a blank string when no walk has occurred' do
      expect(@node.full_state).to eq('')
    end

    it 'returns a non-ASCII string' do
      @node.walk!('c').walk!('a').walk!('f').walk!('é')
      expect(@node.full_state).to eq('café')
    end
  end
  
  describe :walk! do
    it 'returns the updated object when the walk succeeds' do
      other = @node.walk!('r')
      expect(other).to eq(@node)
    end

    it 'returns nil when the walk fails' do
      expect(@node.walk!('q')).to be nil
    end
  end

  describe :walk do
    it 'returns a new node object when the walk succeeds' do
      other = @node.walk('r')
      expect(other).not_to eq(@node)
    end

    it 'returns nil when the walk fails' do
      expect(@node.walk('q')).to be nil
    end
  end

  describe :value do
    it 'returns nil when the node is not terminal' do
      @node.walk!('r')
      expect(@node.value).to be nil
    end

    it 'returns a value when the node is terminal' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      expect(@node.value).to eq(2)
    end
  end

  describe :terminal? do
    it 'returns true when the node is a word end' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      expect(@node).to be_terminal
    end

    it 'returns nil when the node is not a word end' do
      @node.walk!('r').walk!('o').walk!('c')
      expect(@node).not_to be_terminal
    end
  end

  describe :leaf? do
    it 'returns true when this is the end of a branch of the trie' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k').walk!('e').walk!('t')
      expect(@node).to be_leaf
    end

    it 'returns nil when there are more splits on this branch' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      expect(@node).not_to be_leaf
    end
  end

  describe :clone do
    it 'creates a new instance of this node which is not this node' do
      new_node = @node.clone
      expect(new_node).not_to eq(@node)
    end

    it 'matches the state of the current node' do
      new_node = @node.clone
      expect(new_node.state).to eq(@node.state)
    end

    it 'matches the full_state of the current node' do
      new_node = @node.clone
      expect(new_node.full_state).to eq(@node.full_state)
    end
  end
end

describe AlphaMap do
  describe 'empty map' do
    it 'restricts adding any values' do
      trie = Trie.new AlphaMap.new
      expect(trie.add('a')).to be_nil
    end
  end

  describe '.from_ranges' do
    it 'restricts adding values outside of alphabet' do
      trie = Trie.new AlphaMap.from_ranges('a'..'d')
      expect(trie.add('abc')).to be_truthy
      expect(trie.add('def')).to be_nil
    end

    it 'allows merging of ranges' do
      trie = Trie.new AlphaMap.from_ranges('a'..'d', 'd'..'f')
      expect(trie.add('abc')).to be_truthy
      expect(trie.add('def')).to be_truthy
    end

    it 'allows ranges via string' do
      trie = Trie.new AlphaMap.from_ranges('a'..'c', '_')
      expect(trie.add('a_b_c')).to be_truthy
    end

    it 'allows integer ranges' do
      trie = Trie.new AlphaMap.from_ranges(65..68)
      expect(trie.add('ABC')).to be_truthy
    end
  end

  describe '.ascii' do
    it 'only allows ascii values' do
      trie = Trie.new AlphaMap.ascii
      expect(trie.add('english')).to be_truthy
      expect(trie.add('français')).to be_nil
    end
  end
end
