require File.dirname(__FILE__) + '/../lib/trie'

describe Trie do
  before :each do
    @alpha_map = AlphaMap.new
    @alpha_map.add_range 0,255
    @trie = Trie.new(@alpha_map)
    @trie.add('rocket')
    @trie.add('rock')
    @trie.add('frederico')
    @trie.add('français')
  end
  
  describe :has_key? do
    it 'returns true for words in the trie' do
      @trie.has_key?('rocket').should == true
    end

    it 'returns true for non-ASCII words in the trie' do
      @trie.has_key?('français').should == true
    end

    it 'returns nil for words that are not in the trie' do
      @trie.has_key?('not_in_the_trie').should be_nil
    end
  end

  describe :get do
    it 'returns -1 for words in the trie without a weight' do
      @trie.get('rocket').should == -1
    end

    it 'returns -1 for non-ASCII words in the trie without a weight' do
      @trie.get('français').should == -1
    end

    it 'returns nil if the word is not in the trie' do
      @trie.get('not_in_the_trie').should be_nil
    end
  end

  describe :add do
    it 'adds a word to the trie' do
      @trie.add('forsooth').should == true
      @trie.get('forsooth').should == -1
    end

    it 'adds a word with a weight to the trie' do
      @trie.add('chicka',123).should == true
      @trie.get('chicka').should == 123
    end

    it 'adds values greater than 16-bit allows' do
      @trie.add('chicka', 72_000).should == true
      @trie.get('chicka').should == 72_000
    end

    it 'adds values that are not plain ASCII' do
      @trie.add('café', 72_000).should == true
      @trie.get('café').should == 72_000
    end
  end

  describe :add_if_absent do
    it 'adds a word to the trie' do
      @trie.add_if_absent('forsooth').should == true
      @trie.get('forsooth').should == -1
    end

    it 'not add a word that is already present' do
      @trie.add_if_absent('forsooth').should == true
      @trie.add_if_absent('forsooth').should be_nil
    end
  end

  describe :add_text do
    it 'adds multiple words to the trie delimited by spaces' do
      @trie.add_text('forsooth chicka boom', ' ')
      @trie.has_key?('forsooth').should == true
      @trie.has_key?('chicka').should == true
      @trie.has_key?('boom').should == true
      @trie.has_key?('not_there').should be_nil
    end

    it 'adds multiple words to the trie delimited by commas' do
      @trie.add_text('forsooth,chicka,boom', ' ,')
      @trie.has_key?('forsooth').should == true
      @trie.has_key?('chicka').should == true
      @trie.has_key?('boom').should == true
      @trie.has_key?('not_there').should be_nil
    end
  end

  describe :concat do
    it 'adds multiple words to the trie' do
      @trie.concat %w{forsooth chicka boom}
      @trie.has_key?('forsooth').should == true
      @trie.has_key?('chicka').should == true
      @trie.has_key?('boom').should == true
      @trie.has_key?('not_there').should be_nil
    end

    it 'adds multiple words to the trie with weights' do
      @trie.concat [['forsooth', 1], ['chicka', 2], ['boom', 3]]
      @trie.get('forsooth').should == 1
      @trie.get('chicka').should == 2
      @trie.get('boom').should == 3
      @trie.has_key?('not_there').should be_nil
    end
  end

  describe :text_has_keys? do
    it 'scans words in text for keys' do
      @trie.add_text('forsooth rock rocket', ' ')
      @trie.text_has_keys?('the word "forsooth" is in this text').should == true
      @trie.text_has_keys?('yes, rock!').should == true
      @trie.text_has_keys?('3.rocket.4').should == true
      @trie.text_has_keys?('not_there not_there_either').should be_nil
    end
  end

  describe :tags_has_keys? do
    it 'scans words in space-delimited tag list for keys' do
      @trie.add_text('forsooth rock rocket', ' ')
      @trie.tags_has_keys?('banana frobozz rock').should == true
      @trie.tags_has_keys?('rocket abc def').should == true
      @trie.tags_has_keys?('yes forsooth group').should == true
      @trie.tags_has_keys?('not_there not_there_either').should be_nil
    end
  end

  describe :delete do
    it 'deletes a word from the trie' do
      @trie.delete('rocket').should == true
      @trie.has_key?('rocket').should be_nil
    end

    it 'deletes a non-ASCII word from the trie' do
      @trie.delete('français').should == true
      @trie.has_key?('français').should be_nil
    end
  end

  describe :children do
    it 'returns all words beginning with a given prefix' do
      children = @trie.children('roc')
      children.size.should == 2
      children.should include('rock')
      children.should include('rocket')
    end

    it 'accepts non-ASCII prefixes and returns non-ASCII words' do
      @trie.add('café')
      @trie.add('cafétière')
      children = @trie.children('café')
      children.size.should == 2
      children.should include('café')
      children.should include('cafétière')
    end

    it 'returns blank array if prefix does not exist' do
      @trie.children('ajsodij').should == []
    end

    it 'includes the prefix if the prefix is a word' do
      children = @trie.children('rock')
      children.size.should == 2
      children.should include('rock')
      children.should include('rocket')
    end

    it 'returns blank array if prefix is nil' do
      @trie.children(nil).should == []
    end
  end

  describe :children_with_values do
    before :each do
      @trie.add('abc',2)
      @trie.add('abcd',4)
    end

    it 'returns all words with values beginning with a given prefix' do
      children = @trie.children_with_values('ab')
      children.size.should == 2
      children.should include(['abc',2])
      children.should include(['abcd',4])
    end

    it 'returns all words with values beginning with a given non-ASCII prefix' do
      @trie.add('café', 2)
      @trie.add('cafétière', 4)
      children = @trie.children_with_values('café')
      children.size.should == 2
      children.should include(['café',2])
      children.should include(['cafétière',4])
    end

    it 'returns nil if prefix does not exist' do
      @trie.children_with_values('ajsodij').should == []
    end

    it 'includes the prefix if the prefix is a word' do
      children = @trie.children_with_values('abc')
      children.size.should == 2
      children.should include(['abc',2])
      children.should include(['abcd',4])
    end

    it 'returns blank array if prefix is nil' do
      @trie.children_with_values(nil).should == []
    end
  end

  #describe :walk_to_terminal do
  #  it 'returns the first word found along a path' do
  #    @trie.add 'anderson'
  #    @trie.add 'andreas'
  #    @trie.add 'and'

  #    @trie.walk_to_terminal('anderson').should == 'and'
  #  end

  #  it 'returns the first word and value along a path' do
  #    @trie.add 'anderson'
  #    @trie.add 'andreas'
  #    @trie.add 'and', 15

  #    @trie.walk_to_terminal('anderson',true).should == ['and', 15]
  #  end
  #end

  describe :root do
    it 'returns a TrieNode' do
      @trie.root.should be_an_instance_of(TrieNode)
    end

    it 'returns a different TrieNode each time' do
      @trie.root.should_not == @trie.root
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

      it 'should contain the same data when reading from disk' do
        trie2 = Trie.read(filename_base)
        trie2.get('omgwtflolbbq').should == 123
      end
    end
  end

  describe :read do
    context 'when the files to read from do not exist' do
      let(:filename_base) do
        "phantasy/file/path/that/does/not/exist"
      end

      it 'should raise an error when attempting a read' do
        lambda { Trie.read(filename_base) }.should raise_error(IOError)
      end
    end
  end

  describe :has_children? do
    it 'returns true when there are children matching prefix' do
      @trie.has_children?('r').should == true
      @trie.has_children?('rock').should == true
      @trie.has_children?('rocket').should == true
    end

    it 'returns true when there are children matching non-ASCII prefix' do
      @trie.has_children?('franç').should == true
    end

    it 'returns false when there are no children matching prefix' do
      @trie.has_children?('no').should == false
      @trie.has_children?('rome').should == false
      @trie.has_children?('roc_').should == false
    end
  end
end

describe TrieNode do
  before :each do
    @alpha_map = AlphaMap.new
    @alpha_map.add_range 0,255
    @trie = Trie.new(@alpha_map)
    @trie.add('rocket',1)
    @trie.add('rock',2)
    @trie.add('frederico',3)
    @trie.add('café',4)
    @node = @trie.root
  end
  
  describe :state do
    it 'returns the most recent state character' do
      @node.walk!('r')
      @node.state.should == 'r'
      @node.walk!('o')
      @node.state.should == 'o'
    end

    it 'is nil when no walk has occurred' do
      @node.state.should == nil
    end
  end

  describe :full_state do
    it 'returns the current string' do
      @node.walk!('r').walk!('o').walk!('c')
      @node.full_state.should == 'roc'
    end

    it 'is a blank string when no walk has occurred' do
      @node.full_state.should == ''
    end

    it 'returns a non-ASCII string' do
      @node.walk!('c').walk!('a').walk!('f').walk!('é')
      @node.full_state.should == 'café'
    end
  end
  
  describe :walk! do
    it 'returns the updated object when the walk succeeds' do
      other = @node.walk!('r')
      other.should == @node
    end

    it 'returns nil when the walk fails' do
      @node.walk!('q').should be_nil
    end
  end

  describe :walk do
    it 'returns a new node object when the walk succeeds' do
      other = @node.walk('r')
      other.should_not == @node
    end

    it 'returns nil when the walk fails' do
      @node.walk('q').should be_nil
    end
  end

  describe :value do
    it 'returns nil when the node is not terminal' do
      @node.walk!('r')
      @node.value.should be_nil
    end

    it 'returns a value when the node is terminal' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      @node.value.should == 2
    end
  end

  describe :terminal? do
    it 'returns true when the node is a word end' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      @node.should be_terminal
    end

    it 'returns nil when the node is not a word end' do
      @node.walk!('r').walk!('o').walk!('c')
      @node.should_not be_terminal
    end
  end

  describe :leaf? do
    it 'returns true when this is the end of a branch of the trie' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k').walk!('e').walk!('t')
      @node.should be_leaf
    end

    it 'returns nil when there are more splits on this branch' do
      @node.walk!('r').walk!('o').walk!('c').walk!('k')
      @node.should_not be_leaf
    end
  end

  describe :clone do
    it 'creates a new instance of this node which is not this node' do
      new_node = @node.clone
      new_node.should_not == @node
    end

    it 'matches the state of the current node' do
      new_node = @node.clone
      new_node.state.should == @node.state
    end

    it 'matches the full_state of the current node' do
      new_node = @node.clone
      new_node.full_state.should == @node.full_state
    end
  end
end
